#include "ConfigPortal.h"

#include <ArduinoJson.h>
#include <WiFi.h>

#include "PortalAssets.h"

namespace {
constexpr uint8_t DNS_PORT = 53;
constexpr uint8_t MAX_NETWORKS = 20;
}

void ConfigPortal::start(ConnectHandler onConnect, StatusProvider statusProvider) {
  onConnect_ = onConnect;
  statusProvider_ = statusProvider;

  if (!routesReady_) {
    server_.on("/", HTTP_GET, [this]() { handleRoot(); });
    server_.on("/style.css", HTTP_GET, [this]() { sendAsset("text/css", PORTAL_CSS); });
    server_.on("/app.js", HTTP_GET, [this]() { sendAsset("application/javascript", PORTAL_JS); });
    server_.on("/scan", HTTP_GET, [this]() { handleScan(); });
    server_.on("/connect", HTTP_POST, [this]() { handleConnect(); });
    server_.on("/status", HTTP_GET, [this]() { handleStatus(); });
    server_.onNotFound([this]() { handleNotFound(); });
    routesReady_ = true;
  }

  dns_.start(DNS_PORT, "*", WiFi.softAPIP());
  server_.begin();
  running_ = true;
  Serial.printf("[Portal] Started at http://%s\n", WiFi.softAPIP().toString().c_str());
}

void ConfigPortal::stop() {
  if (!running_) {
    return;
  }
  server_.stop();
  dns_.stop();
  running_ = false;
  Serial.println("[Portal] Stopped");
}

void ConfigPortal::loop() {
  if (!running_) {
    return;
  }
  dns_.processNextRequest();
  server_.handleClient();
}

void ConfigPortal::handleRoot() {
  sendAsset("text/html; charset=utf-8", PORTAL_HTML);
}

void ConfigPortal::sendAsset(const char* contentType, const char* body) {
  server_.sendHeader("Cache-Control", "no-store");
  server_.send_P(200, contentType, body);
}

void ConfigPortal::handleScan() {
  int16_t count = WiFi.scanComplete();
  bool refresh = server_.hasArg("refresh");

  if (count >= 0 && refresh) {
    WiFi.scanDelete();
    count = WIFI_SCAN_FAILED;
  }
  if (count == WIFI_SCAN_FAILED) {
    WiFi.scanNetworks(true);
    server_.send(200, "application/json", "{\"scanning\":true}");
    return;
  }
  if (count == WIFI_SCAN_RUNNING) {
    server_.send(200, "application/json", "{\"scanning\":true}");
    return;
  }

  JsonDocument doc;
  doc["scanning"] = false;
  JsonArray list = doc["networks"].to<JsonArray>();

  for (int16_t i = 0; i < count && list.size() < MAX_NETWORKS; i++) {
    String ssid = WiFi.SSID(i);
    if (ssid.isEmpty()) {
      continue;
    }
    int32_t rssi = WiFi.RSSI(i);
    bool duplicate = false;
    for (JsonObject item : list) {
      if (ssid == item["ssid"].as<const char*>()) {
        duplicate = true;
        if (rssi > item["rssi"].as<int32_t>()) {
          item["rssi"] = rssi;
        }
        break;
      }
    }
    if (duplicate) {
      continue;
    }
    JsonObject item = list.add<JsonObject>();
    item["ssid"] = ssid;
    item["rssi"] = rssi;
    item["secure"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
  }
  WiFi.scanDelete();

  String body;
  serializeJson(doc, body);
  server_.send(200, "application/json", body);
}

void ConfigPortal::handleConnect() {
  String ssid = server_.arg("ssid");
  String password = server_.arg("password");
  ssid.trim();

  if (ssid.isEmpty() || ssid.length() > 32 || password.length() > 64) {
    server_.send(400, "application/json", "{\"ok\":false}");
    return;
  }
  server_.send(200, "application/json", "{\"ok\":true}");
  if (onConnect_) {
    onConnect_(ssid, password);
  }
}

void ConfigPortal::handleStatus() {
  server_.sendHeader("Cache-Control", "no-store");
  server_.send(200, "application/json", statusProvider_ ? statusProvider_() : String("{}"));
}

void ConfigPortal::handleNotFound() {
  server_.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/", true);
  server_.send(302, "text/plain", "");
}
