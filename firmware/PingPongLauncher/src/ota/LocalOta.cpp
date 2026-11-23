#include "LocalOta.h"

#include <ArduinoOTA.h>

#include "../config/AppConfig.h"

void LocalOta::begin(const String& deviceId, StartHandler onStart) {
  if (started_) {
    return;
  }
  String hostname = "pingpong-" + deviceId;
  ArduinoOTA.setHostname(hostname.c_str());
  ArduinoOTA.setPassword(AppConfig::OTA_LOCAL_PASSWORD);
  ArduinoOTA.onStart([onStart]() {
    Serial.println("[OTA] Local upload started");
    if (onStart) {
      onStart();
    }
  });
  ArduinoOTA.onEnd([]() { Serial.println("[OTA] Local upload finished, rebooting"); });
  ArduinoOTA.onError([](ota_error_t error) { Serial.printf("[OTA] Local upload error %u\n", error); });
  ArduinoOTA.begin();
  started_ = true;
  Serial.printf("[OTA] Local OTA ready as %s.local\n", hostname.c_str());
}

void LocalOta::handle() {
  if (started_) {
    ArduinoOTA.handle();
  }
}
