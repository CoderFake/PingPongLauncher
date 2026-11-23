#include "OtaCoordinator.h"

#include <ArduinoJson.h>

#include "../config/AppConfig.h"

void OtaCoordinator::bootCheck() {
  guard_.begin();
}

void OtaCoordinator::attach(MqttLink& mqtt) {
  mqtt_ = &mqtt;
}

void OtaCoordinator::request(const OtaRequest& request, bool launcherPowered) {
  if (busy()) {
    return;
  }
  request_ = request;
  if (launcherPowered) {
    report("error", 0, "power_on");
    return;
  }
  String error = OtaUpdater::validate(request_);
  if (!error.isEmpty()) {
    report("error", 0, error.c_str());
    return;
  }
  if (request_.version == FIRMWARE_VERSION) {
    report("error", 0, "same_version");
    return;
  }
  pending_ = true;
  report("queued", 0, nullptr);
}

void OtaCoordinator::update() {
  if (!pending_ || running_) {
    return;
  }
  pending_ = false;
  running_ = true;
  report("downloading", 0, nullptr);

  OtaResult result = updater_.run(request_, [this](uint8_t percent) {
    report("downloading", percent, nullptr);
    if (mqtt_) {
      mqtt_->pump();
    }
  });

  if (!result.ok) {
    running_ = false;
    report("error", 0, result.error.c_str());
    return;
  }

  guard_.armTrial();
  report("rebooting", 100, nullptr);
  if (mqtt_) {
    mqtt_->pump();
  }
  delay(500);
  ESP.restart();
}

void OtaCoordinator::report(const char* state, int progress, const char* error) {
  if (!mqtt_) {
    return;
  }
  JsonDocument doc;
  doc["state"] = state;
  doc["progress"] = progress;
  doc["version"] = request_.version;
  doc["current"] = FIRMWARE_VERSION;
  if (error != nullptr) {
    doc["error"] = error;
  }
  char json[192];
  serializeJson(doc, json, sizeof(json));
  mqtt_->publishOta(json);
}
