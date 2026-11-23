#include "OtaUpdater.h"

#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <SHA2Builder.h>
#include <Update.h>

#include "../config/AppConfig.h"

namespace {

constexpr size_t CHUNK_SIZE = 2048;

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

  NetworkClientSecure client;
#if ESP_ARDUINO_VERSION_MAJOR >= 4
  client.useBuiltinCACertBundle();
#else
  client.setInsecure();
#endif

  HTTPClient http;
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  http.setTimeout(AppConfig::OTA_HTTP_TIMEOUT_MS);
  http.setUserAgent("pingpong-launcher/" FIRMWARE_VERSION);
  if (!http.begin(client, request.url)) {
    return fail(http, false, "http_begin");
  }

  int status = http.GET();
  if (status != HTTP_CODE_OK) {
    Serial.printf("[OTA] HTTP status %d\n", status);
    return fail(http, false, "http_status");
  }

  int total = http.getSize();
  if (total <= 0) {
    return fail(http, false, "no_length");
  }
  if (!Update.begin(total, U_FLASH)) {
    return fail(http, false, "no_space");
  }

  SHA2Builder sha(32);
  sha.begin();

  NetworkClient* stream = http.getStreamPtr();
  uint8_t buffer[CHUNK_SIZE];
  size_t written = 0;
  uint32_t lastDataMs = millis();
  int lastReported = -1;

  while (written < static_cast<size_t>(total)) {
    size_t available = stream->available();
    if (available == 0) {
      if (!stream->connected() || millis() - lastDataMs > AppConfig::OTA_HTTP_TIMEOUT_MS) {
        return fail(http, true, "download_timeout");
      }
      delay(2);
      continue;
    }

    size_t want = min(min(available, CHUNK_SIZE), static_cast<size_t>(total) - written);
    int read = stream->readBytes(buffer, want);
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
  Serial.printf("[OTA] Image %s verified (%u bytes)\n", request.version.c_str(),
                static_cast<unsigned>(written));
  return {true, ""};
}
