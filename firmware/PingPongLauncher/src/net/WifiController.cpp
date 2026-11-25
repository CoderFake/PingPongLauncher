#include "WifiController.h"

#include <ArduinoJson.h>
#include <WiFi.h>

#include "../config/AppConfig.h"

namespace {
constexpr uint8_t REASON_AUTH_EXPIRE = 2;
constexpr uint8_t REASON_4WAY_HANDSHAKE_TIMEOUT = 15;
constexpr uint8_t REASON_NO_AP_FOUND = 201;
constexpr uint8_t REASON_AUTH_FAIL = 202;
constexpr uint8_t REASON_HANDSHAKE_TIMEOUT = 204;
constexpr uint8_t REASON_NO_AP_COMPATIBLE_SECURITY = 210;
constexpr uint8_t REASON_NO_AP_AUTHMODE_THRESHOLD = 211;
constexpr uint32_t SCAN_WAIT_MS = 4000;
constexpr uint32_t RETRY_AFTER_MS = 9000;
constexpr uint32_t SAVED_RETRY_EVERY_MS = 30000;
constexpr uint32_t SAVED_ATTEMPT_MS = 12000;
constexpr uint32_t LOST_PORTAL_AFTER_MS = 60000;

/**
 * Returns true for disconnect reasons that mean the password was rejected.
 */
bool isAuthReason(uint8_t reason) {
  return reason == REASON_AUTH_FAIL || reason == REASON_4WAY_HANDSHAKE_TIMEOUT ||
         reason == REASON_HANDSHAKE_TIMEOUT || reason == REASON_AUTH_EXPIRE;
}

/**
 * Returns true for disconnect reasons that mean the network was not found.
 */
bool isNoApReason(uint8_t reason) {
  return reason == REASON_NO_AP_FOUND || reason == REASON_NO_AP_COMPATIBLE_SECURITY;
}

/**
 * Returns true when the AP was seen but its security is below the accepted minimum.
 */
bool isSecurityReason(uint8_t reason) {
  return reason == REASON_NO_AP_AUTHMODE_THRESHOLD;
}
}

void WifiController::begin(const String& deviceId) {
  deviceId_ = deviceId;
  apSsid_ = String(AppConfig::AP_SSID_PREFIX) + deviceId_;

  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.setMinSecurity(WIFI_AUTH_WEP);
  WiFi.setSleep(false);
  WiFi.onEvent(
      [this](arduino_event_id_t, arduino_event_info_t info) {
        uint8_t reason = info.wifi_sta_disconnected.reason;
        lastReason_ = reason;
        if (isAuthReason(reason)) {
          authFailures_ = authFailures_ + 1;
        }
      },
      ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  WiFi.setHostname((String("pingpong-") + deviceId_).c_str());

  hasSaved_ = store_.load(savedSsid_, savedPassword_);
  if (!hasSaved_) {
    Serial.println("[WiFi] No saved credentials");
    startPortal();
    return;
  }

  Serial.printf("[WiFi] Connecting to saved network '%s'\n", savedSsid_.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(savedSsid_.c_str(), savedPassword_.c_str());
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
      if (hasSaved_) {
        retrySavedNetwork(nowMs);
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
      if (WiFi.status() == WL_CONNECTED) {
        lostSinceMs_ = 0;
      } else if (lostSinceMs_ == 0) {
        lostSinceMs_ = nowMs;
      } else if (nowMs - lostSinceMs_ >= LOST_PORTAL_AFTER_MS) {
        Serial.println("[WiFi] Network lost for too long, opening portal");
        lostSinceMs_ = 0;
        startPortal();
      }
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
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(false, false);
  savedAttempting_ = false;
  savedAttemptMs_ = millis();
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
  WiFi.setAutoReconnect(true);
  Serial.printf("[WiFi] Switched to STA, IP %s\n", WiFi.localIP().toString().c_str());
  setPhase(Phase::Online);
}

void WifiController::retrySavedNetwork(uint32_t nowMs) {
  bool clientOnAp = WiFi.softAPgetStationNum() > 0;
  if (savedAttempting_) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("[WiFi] Saved network came back, closing portal");
      savedAttempting_ = false;
      finishPortal();
    } else if (clientOnAp || nowMs - savedAttemptMs_ >= SAVED_ATTEMPT_MS) {
      WiFi.disconnect(false, false);
      savedAttempting_ = false;
      savedAttemptMs_ = nowMs;
    }
    return;
  }
  if (!clientOnAp && nowMs - savedAttemptMs_ >= SAVED_RETRY_EVERY_MS &&
      WiFi.scanComplete() != WIFI_SCAN_RUNNING) {
    Serial.printf("[WiFi] Retrying saved network '%s'\n", savedSsid_.c_str());
    WiFi.begin(savedSsid_.c_str(), savedPassword_.c_str());
    savedAttempting_ = true;
    savedAttemptMs_ = nowMs;
  }
}

void WifiController::requestConnect(const String& ssid, const String& password) {
  if (phase_ == Phase::PortalConnecting || phase_ == Phase::PortalSuccess) {
    return;
  }
  savedAttempting_ = false;
  pendingSsid_ = ssid;
  pendingPassword_ = password;
  lastError_ = "";
  targetKnown_ = portal_.lookup(ssid, targetChannel_, targetBssid_);
  attemptStarted_ = false;
  retried_ = false;
  Serial.printf("[WiFi] Portal requested '%s' (channel %ld)\n", ssid.c_str(),
                targetKnown_ ? static_cast<long>(targetChannel_) : -1L);
  setPhase(Phase::PortalConnecting);
}

void WifiController::startAttempt(bool pinToScannedAp) {
  lastReason_ = 0;
  authFailures_ = 0;
  WiFi.disconnect(false, false);
  delay(50);
  const char* password = pendingPassword_.length() ? pendingPassword_.c_str() : nullptr;
  if (pinToScannedAp) {
    WiFi.begin(pendingSsid_.c_str(), password, targetChannel_, targetBssid_);
  } else {
    WiFi.begin(pendingSsid_.c_str(), password);
  }
  attemptStartMs_ = millis();
}

void WifiController::checkPortalAttempt(uint32_t nowMs) {
  uint32_t elapsed = nowMs - phaseStartMs_;

  if (!attemptStarted_) {
    if (WiFi.scanComplete() == WIFI_SCAN_RUNNING && elapsed < SCAN_WAIT_MS) {
      return;
    }
    WiFi.scanDelete();
    startAttempt(targetKnown_);
    attemptStarted_ = true;
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    store_.save(pendingSsid_, pendingPassword_);
    hasSaved_ = true;
    savedSsid_ = pendingSsid_;
    savedPassword_ = pendingPassword_;
    Serial.printf("[WiFi] Joined '%s', IP %s\n", pendingSsid_.c_str(),
                  WiFi.localIP().toString().c_str());
    setPhase(Phase::PortalSuccess);
    return;
  }

  uint8_t reason = lastReason_;
  const char* error = nullptr;
  if (authFailures_ >= 2) {
    error = "auth_failed";
  } else if (!retried_ && (isNoApReason(reason) || nowMs - attemptStartMs_ >= RETRY_AFTER_MS)) {
    Serial.printf("[WiFi] Retrying without channel lock (last reason %u)\n", reason);
    retried_ = true;
    startAttempt(false);
    return;
  } else if (elapsed >= AppConfig::STA_CONNECT_TIMEOUT_MS) {
    error = isSecurityReason(reason) ? "weak_security"
            : isNoApReason(reason)   ? "no_ssid"
            : authFailures_ > 0      ? "auth_failed"
                                     : "timeout";
  }
  if (error == nullptr) {
    return;
  }

  lastError_ = error;
  Serial.printf("[WiFi] Join failed: %s (reason %u)\n", error, reason);
  WiFi.disconnect(false, false);
  setPhase(Phase::Portal);
}

WifiStatus WifiController::status() const {
  WifiStatus out{WifiState::Connecting, "", "", apSsid_, lastError_};
  bool linked = WiFi.status() == WL_CONNECTED;
  switch (phase_) {
    case Phase::StaConnecting:
      out.state = WifiState::Connecting;
      out.ssid = WiFi.SSID();
      break;
    case Phase::Portal:
      out.state = lastError_.isEmpty() ? WifiState::Portal : WifiState::JoinFailed;
      out.ssid = pendingSsid_;
      break;
    case Phase::PortalConnecting:
      out.state = WifiState::Joining;
      out.ssid = pendingSsid_;
      break;
    case Phase::PortalSuccess:
      out.state = WifiState::Joined;
      break;
    case Phase::Online:
      out.state = linked ? WifiState::Online : WifiState::Lost;
      break;
  }
  if (linked) {
    out.ssid = WiFi.SSID();
    out.ip = WiFi.localIP().toString();
  }
  return out;
}

String WifiController::statusJson() const {
  JsonDocument doc;
  doc["deviceId"] = deviceId_;
  doc["ap"] = apSsid_;
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
