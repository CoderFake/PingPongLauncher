#include "OtaGuard.h"

#include <Preferences.h>
#include <esp_ota_ops.h>

#include "../config/AppConfig.h"

namespace {
constexpr const char* NAMESPACE = "ota";
constexpr const char* KEY_TRIAL = "trial";
constexpr const char* KEY_BOOTS = "boots";
}

void OtaGuard::begin() {
  Preferences prefs;
  prefs.begin(NAMESPACE, false);
  trial_ = prefs.getBool(KEY_TRIAL, false);
  if (!trial_) {
    prefs.end();
    return;
  }

  uint8_t boots = prefs.getUChar(KEY_BOOTS, 0) + 1;
  prefs.putUChar(KEY_BOOTS, boots);
  Serial.printf("[OTA] Trial boot %u/%u\n", boots, AppConfig::OTA_MAX_TRIAL_BOOTS);

  if (boots <= AppConfig::OTA_MAX_TRIAL_BOOTS) {
    prefs.end();
    return;
  }

  prefs.putBool(KEY_TRIAL, false);
  prefs.putUChar(KEY_BOOTS, 0);
  prefs.end();
  trial_ = false;

  const esp_partition_t* previous = esp_ota_get_next_update_partition(nullptr);
  esp_app_desc_t desc;
  if (previous != nullptr && esp_ota_get_partition_description(previous, &desc) == ESP_OK) {
    Serial.printf("[OTA] New image unhealthy, rolling back to %s\n", desc.version);
    esp_ota_set_boot_partition(previous);
    delay(100);
    ESP.restart();
  }
  Serial.println("[OTA] No previous image to roll back to");
}

void OtaGuard::armTrial() {
  Preferences prefs;
  prefs.begin(NAMESPACE, false);
  prefs.putBool(KEY_TRIAL, true);
  prefs.putUChar(KEY_BOOTS, 0);
  prefs.end();
}

void OtaGuard::markHealthy() {
  if (!trial_) {
    return;
  }
  Preferences prefs;
  prefs.begin(NAMESPACE, false);
  prefs.putBool(KEY_TRIAL, false);
  prefs.putUChar(KEY_BOOTS, 0);
  prefs.end();
  trial_ = false;
  Serial.println("[OTA] New image confirmed healthy");
}
