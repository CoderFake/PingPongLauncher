#include "WifiController.h"

#include <ArduinoJson.h>
#include <WiFi.h>

#include "../config/AppConfig.h"

void WifiController::begin(const String& deviceId) {
  deviceId_ = deviceId;
  apSsid_ = String(AppConfig::AP_SSID_PREFIX) + deviceId_;

  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.setHostname((String("pingpong-") + deviceId_).c_str());

  String ssid;
  String password;
  hasSaved_ = store_.load(ssid, password);
  if (!hasSaved_) {
    Serial.println("[WiFi] No saved credentials");
    startPortal();
    return;
  }

  Serial.printf("[WiFi] Connecting to saved network '%s'\n", ssid.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  setPhase(Phase::StaConnecting);
}

void WifiController::update(uint32_t nowMs) {
  switch (phase_) {
    case Phase::StaConnecting:
      if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[WiFi] Online, IP %s\n", WiFi.localIP().toString().c_str());
        setPhase(Phase::Online);
      } else if (nowMs - phaseStartMs_ >= AppConfig::STA_CONNECT_TIMEOUT_MS) {
        Serial.println("[WiFi] Saved network unreachable, opening portal");
        startPortal();
      }
      break;
    case Phase::Portal:
      portal_.loop();
      if (hasSaved_ && WiFi.status() == WL_CONNECTED) {
        Serial.println("[WiFi] Saved network came back, closing portal");
        finishPortal();
      }
      break;
    case Phase::PortalConnecting:
      portal_.loop();
      checkPortalAttempt(nowMs);
      break;
    case Phase::PortalSuccess:
      portal_.loop();
      if (nowMs - phaseStartMs_ >= AppConfig::PORTAL_SWITCH_DELAY_MS) {
        finishPortal();
      }
      break;
    case Phase::Online:
      break;
  }
}

bool WifiController::isOnline() const {
  return (phase_ == Phase::Online || phase_ == Phase::PortalSuccess) &&
         WiFi.status() == WL_CONNECTED;
}

void WifiController::resetCredentials() {
  Serial.println("[WiFi] Clearing credentials and rebooting");
  store_.clear();
  WiFi.disconnect(true, true);
  delay(200);
  ESP.restart();
}

void WifiController::startPortal() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(apSsid_.c_str(), AppConfig::AP_PASSWORD);
  delay(100);
  portal_.start([this](const String& ssid, const String& password) { requestConnect(ssid, password); },
                [this]() { return statusJson(); });
  lastError_ = "";
  Serial.printf("[WiFi] Portal AP '%s' up\n", apSsid_.c_str());
  setPhase(Phase::Portal);
}

void WifiController::finishPortal() {
  portal_.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  Serial.printf("[WiFi] Switched to STA, IP %s\n", WiFi.localIP().toString().c_str());
  setPhase(Phase::Online);
}

void WifiController::requestConnect(const String& ssid, const String& password) {
  if (phase_ == Phase::PortalConnecting || phase_ == Phase::PortalSuccess) {
    return;
  }
  pendingSsid_ = ssid;
  pendingPassword_ = password;
  lastError_ = "";
  Serial.printf("[WiFi] Portal requested '%s'\n", ssid.c_str());

  WiFi.scanDelete();
  WiFi.disconnect(false, false);
  WiFi.begin(ssid.c_str(), password.length() ? password.c_str() : nullptr);
  setPhase(Phase::PortalConnecting);
}

void WifiController::checkPortalAttempt(uint32_t nowMs) {
  wl_status_t status = WiFi.status();
  if (status == WL_CONNECTED) {
    store_.save(pendingSsid_, pendingPassword_);
    hasSaved_ = true;
    Serial.printf("[WiFi] Joined '%s', IP %s\n", pendingSsid_.c_str(),
                  WiFi.localIP().toString().c_str());
    setPhase(Phase::PortalSuccess);
    return;
  }

  bool timedOut = nowMs - phaseStartMs_ >= AppConfig::STA_CONNECT_TIMEOUT_MS;
  if (status == WL_NO_SSID_AVAIL && nowMs - phaseStartMs_ > 5000) {
    lastError_ = "no_ssid";
  } else if (status == WL_CONNECT_FAILED) {
    lastError_ = "auth_failed";
  } else if (timedOut) {
    lastError_ = "timeout";
  } else {
    return;
  }

  Serial.printf("[WiFi] Join failed: %s\n", lastError_.c_str());
  WiFi.disconnect(false, false);
  setPhase(Phase::Portal);
}

String WifiController::statusJson() const {
  JsonDocument doc;
  doc["deviceId"] = deviceId_;
  switch (phase_) {
    case Phase::PortalConnecting:
      doc["state"] = "connecting";
      break;
    case Phase::PortalSuccess:
    case Phase::Online:
      doc["state"] = "connected";
      doc["ssid"] = WiFi.SSID();
      doc["ip"] = WiFi.localIP().toString();
      break;
    default:
      doc["state"] = lastError_.isEmpty() ? "idle" : "failed";
      doc["error"] = lastError_;
      break;
  }
  String out;
  serializeJson(doc, out);
  return out;
}

void WifiController::setPhase(Phase phase) {
  phase_ = phase;
  phaseStartMs_ = millis();
}
