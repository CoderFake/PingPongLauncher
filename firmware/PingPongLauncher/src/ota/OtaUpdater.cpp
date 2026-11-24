#include "OtaUpdater.h"

#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <SHA2Builder.h>
#include <Update.h>
#include <WiFi.h>

#include <memory>

#include "../config/AppConfig.h"

namespace {

constexpr size_t CHUNK_SIZE = 2048;
constexpr uint8_t MAX_REDIRECTS = 5;
constexpr uint8_t MAX_RESUMES = 4;
constexpr uint32_t STALL_TIMEOUT_MS = 10000;

/**
 * Returns true when `text` is a 64-character hexadecimal string.
 */
bool isSha256Hex(const String& text) {
  if (text.length() != 64) {
    return false;
  }
  for (size_t i = 0; i < text.length(); i++) {
    if (!isHexadecimalDigit(text[i])) {
      return false;
    }
  }
  return true;
}

/**
 * Aborts a started update and builds a failed result.
 */
OtaResult fail(HTTPClient& http, bool updateStarted, const char* code) {
  if (updateStarted) {
    Update.abort();
  }
  http.end();
  Serial.printf("[OTA] Failed: %s\n", code);
  return {false, code};
}

/**
 * Returns true for HTTP status codes that carry a Location to follow.
 */
bool isRedirect(int status) {
  return status == 301 || status == 302 || status == 303 || status == 307 || status == 308;
}

/**
 * Creates a TLS client that verifies servers against the built-in CA bundle.
 */
std::unique_ptr<NetworkClientSecure> makeTlsClient() {
  std::unique_ptr<NetworkClientSecure> client(new NetworkClientSecure());
#if ESP_ARDUINO_VERSION_MAJOR >= 4
  client->useBuiltinCACertBundle();
#else
  client->setInsecure();
#endif
  client->setHandshakeTimeout(AppConfig::OTA_HTTP_TIMEOUT_MS / 1000);
  return client;
}

/**
 * GETs `url`, following redirects by hand. Every hop gets a brand-new TLS
 * client, because GitHub redirects release downloads to another host and
 * reusing the github.com connection for it fails. `client` keeps the final
 * connection alive for streaming and `finalUrl` receives the last URL so a
 * stalled download can be resumed there with a Range request from `offset`.
 * Returns the final HTTP status.
 */
int openDownload(HTTPClient& http, std::unique_ptr<NetworkClientSecure>& client, String url,
                 String& finalUrl, size_t offset) {
  static const char* HEADERS[] = {"Location"};
  for (uint8_t hop = 0; hop <= MAX_REDIRECTS; hop++) {
    client = makeTlsClient();
    Serial.printf("[OTA] Hop %u, heap %u (max block %u)\n", hop,
                  static_cast<unsigned>(ESP.getFreeHeap()),
                  static_cast<unsigned>(ESP.getMaxAllocHeap()));
    http.setReuse(false);
    http.setConnectTimeout(AppConfig::OTA_HTTP_TIMEOUT_MS);
    http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
    http.setTimeout(AppConfig::OTA_HTTP_TIMEOUT_MS);
    http.setUserAgent("pingpong-launcher/" FIRMWARE_VERSION);
    http.collectHeaders(HEADERS, 1);
    if (!http.begin(*client, url)) {
      return -1;
    }
    if (offset > 0) {
      http.addHeader("Range", String("bytes=") + offset + "-");
    }
    finalUrl = url;
    int status = http.GET();
    if (status < 0) {
      char tlsError[96] = "";
      client->lastError(tlsError, sizeof(tlsError));
      Serial.printf("[OTA] Request failed %d (%s), TLS: %s\n", status,
                    HTTPClient::errorToString(status).c_str(), tlsError);
    }
    if (!isRedirect(status)) {
      return status;
    }
    String next = http.header("Location");
    http.end();
    client->stop();
    if (next.isEmpty()) {
      return status;
    }
    Serial.printf("[OTA] Redirect %d -> %.60s...\n", status, next.c_str());
    url = next;
  }
  return -2;
}

}

String OtaUpdater::validate(const OtaRequest& request) {
  if (!request.url.startsWith(AppConfig::OTA_URL_PREFIX)) {
    return "url_not_allowed";
  }
  if (!isSha256Hex(request.sha256)) {
    return "bad_sha256";
  }
  if (request.version.isEmpty() || request.version.length() > 24) {
    return "bad_version";
  }
  return "";
}

OtaResult OtaUpdater::run(const OtaRequest& request, ProgressCallback onProgress) {
  Serial.printf("[OTA] Downloading %s\n", request.url.c_str());
  WiFi.setSleep(false);

  std::unique_ptr<NetworkClientSecure> client;
  HTTPClient http;
  String assetUrl;

  int status = openDownload(http, client, request.url, assetUrl, 0);
  if (status != HTTP_CODE_OK) {
    Serial.printf("[OTA] HTTP status %d\n", status);
    http.end();
    return {false, String("http_") + status};
  }

  const int total = http.getSize();
  if (total <= 0) {
    return fail(http, false, "no_length");
  }
  if (!Update.begin(total, U_FLASH)) {
    return fail(http, false, "no_space");
  }

  SHA2Builder sha(32);
  sha.begin();

  uint8_t buffer[CHUNK_SIZE];
  size_t written = 0;
  uint8_t resumes = 0;
  int lastReported = -1;
  int lastLogged = -1;
  const uint32_t startMs = millis();
  uint32_t lastDataMs = startMs;
  NetworkClient* stream = http.getStreamPtr();

  while (written < static_cast<size_t>(total)) {
    size_t available = stream->available();
    if (available == 0) {
      bool stalled = millis() - lastDataMs > STALL_TIMEOUT_MS;
      if (stream->connected() && !stalled) {
        delay(2);
        continue;
      }
      if (resumes >= MAX_RESUMES) {
        Serial.printf("[OTA] Stalled at %u/%d bytes, giving up\n", static_cast<unsigned>(written), total);
        return fail(http, true, "download_timeout");
      }
      resumes++;
      Serial.printf("[OTA] Stalled at %u/%d bytes (%s), resume %u/%u\n",
                    static_cast<unsigned>(written), total, stalled ? "no data" : "closed", resumes,
                    MAX_RESUMES);
      http.end();
      status = openDownload(http, client, assetUrl, assetUrl, written);
      if (status != HTTP_CODE_PARTIAL_CONTENT ||
          http.getSize() != total - static_cast<int>(written)) {
        Serial.printf("[OTA] Resume rejected: status %d, size %d\n", status, http.getSize());
        return fail(http, true, "resume_failed");
      }
      stream = http.getStreamPtr();
      lastDataMs = millis();
      continue;
    }

    size_t want = min(min(available, CHUNK_SIZE), static_cast<size_t>(total) - written);
    int read = stream->read(buffer, want);
    if (read <= 0) {
      continue;
    }
    sha.add(buffer, read);
    if (Update.write(buffer, read) != static_cast<size_t>(read)) {
      return fail(http, true, "flash_write");
    }
    written += read;
    lastDataMs = millis();

    int percent = written * 100 / total;
    if (percent / 5 != lastReported / 5 && onProgress) {
      lastReported = percent;
      onProgress(percent);
    }
    if (percent / 10 != lastLogged / 10) {
      lastLogged = percent;
      uint32_t elapsed = max<uint32_t>(1, millis() - startMs);
      Serial.printf("[OTA] %3d%%  %u bytes  %u KB/s\n", percent, static_cast<unsigned>(written),
                    static_cast<unsigned>(written / elapsed));
    }
  }

  sha.calculate();
  String digest = sha.toString();
  if (!digest.equalsIgnoreCase(request.sha256)) {
    Serial.printf("[OTA] SHA-256 mismatch, got %s\n", digest.c_str());
    return fail(http, true, "sha_mismatch");
  }
  if (!Update.end()) {
    return fail(http, false, "finalize");
  }

  http.end();
  Serial.printf("[OTA] Image %s verified (%u bytes, %lus)\n", request.version.c_str(),
                static_cast<unsigned>(written), static_cast<unsigned long>((millis() - startMs) / 1000));
  return {true, ""};
}
