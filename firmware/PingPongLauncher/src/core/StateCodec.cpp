#include "StateCodec.h"

#include <ArduinoJson.h>

namespace {

/**
 * Reads an integer field and clamps it into [minValue, maxValue].
 */
template <typename T>
bool readClamped(JsonDocument& doc, const char* key, int minValue, int maxValue, T& target) {
  JsonVariant value = doc[key];
  if (!value.is<int>()) {
    return false;
  }
  int next = constrain(value.as<int>(), minValue, maxValue);
  if (static_cast<int>(target) == next) {
    return false;
  }
  target = static_cast<T>(next);
  return true;
}

}

CommandResult StateCodec::applyCommand(const uint8_t* payload, size_t length,
                                       LauncherState& state, OtaRequest& ota) {
  JsonDocument doc;
  if (deserializeJson(doc, payload, length)) {
    return CommandResult::Invalid;
  }

  const char* id = doc["id"];
  if (id == nullptr || strlen(id) == 0 || strlen(id) > ID_MAX_LEN) {
    return CommandResult::Invalid;
  }
  if (seenRecently(id)) {
    return CommandResult::Duplicate;
  }
  remember(id);

  if (doc["get"].is<bool>() && doc["get"].as<bool>()) {
    return CommandResult::StateRequested;
  }

  JsonObject otaCmd = doc["ota"];
  if (!otaCmd.isNull()) {
    ota.url = otaCmd["url"] | "";
    ota.sha256 = otaCmd["sha256"] | "";
    ota.version = otaCmd["version"] | "";
    return CommandResult::OtaRequested;
  }

  bool changed = false;

  if (doc["power"].is<bool>()) {
    bool next = doc["power"].as<bool>();
    if (next != state.power) {
      state.power = next;
      changed = true;
    }
  }

  if (doc["sbAll"].is<int>()) {
    uint8_t all = constrain(doc["sbAll"].as<int>(), 0, 100);
    if (state.speedTop != all || state.speedBottom != all) {
      state.speedTop = all;
      state.speedBottom = all;
      changed = true;
    }
  }
  changed |= readClamped(doc, "sb1", 0, 100, state.speedTop);
  changed |= readClamped(doc, "sb2", 0, 100, state.speedBottom);

  const char* modeText = doc["mode"];
  TurretMode mode;
  if (modeText != nullptr && modeFromString(modeText, mode) && mode != state.turretMode) {
    state.turretMode = mode;
    changed = true;
  }

  changed |= readClamped(doc, "range", AppConfig::TURRET_RANGE_MIN, AppConfig::TURRET_LIMIT_DEG,
                         state.turretRange);
  changed |= readClamped(doc, "angle", -AppConfig::TURRET_LIMIT_DEG, AppConfig::TURRET_LIMIT_DEG,
                         state.turretAngle);
  changed |= readClamped(doc, "feed", AppConfig::FEED_RATE_MIN, AppConfig::FEED_RATE_MAX,
                         state.feedRate);

  if (!changed) {
    return CommandResult::NoChange;
  }
  state.rev++;
  return CommandResult::Changed;
}

size_t StateCodec::serialize(const LauncherState& state, const LauncherTelemetry& telemetry,
                             char* out, size_t capacity) const {
  JsonDocument doc;
  doc["rev"] = state.rev;
  doc["power"] = state.power;
  doc["sb1"] = state.speedTop;
  doc["sb2"] = state.speedBottom;
  doc["mode"] = modeToString(state.turretMode);
  doc["range"] = state.turretRange;
  doc["angle"] = state.turretAngle;
  doc["feed"] = state.feedRate;
  doc["ready"] = telemetry.ready;
  doc["shots"] = telemetry.shots;
  doc["boot"] = telemetry.bootId;
  doc["fw"] = FIRMWARE_VERSION;
  return serializeJson(doc, out, capacity);
}

const char* StateCodec::modeToString(TurretMode mode) {
  switch (mode) {
    case TurretMode::Random:
      return "random";
    case TurretMode::Fixed:
      return "fixed";
    default:
      return "sweep";
  }
}

bool StateCodec::modeFromString(const char* text, TurretMode& mode) {
  if (strcmp(text, "sweep") == 0) {
    mode = TurretMode::Sweep;
  } else if (strcmp(text, "random") == 0) {
    mode = TurretMode::Random;
  } else if (strcmp(text, "fixed") == 0) {
    mode = TurretMode::Fixed;
  } else {
    return false;
  }
  return true;
}

bool StateCodec::seenRecently(const char* id) const {
  for (uint8_t i = 0; i < ID_HISTORY; i++) {
    if (strcmp(recentIds_[i], id) == 0) {
      return true;
    }
  }
  return false;
}

void StateCodec::remember(const char* id) {
  strncpy(recentIds_[nextSlot_], id, ID_MAX_LEN);
  recentIds_[nextSlot_][ID_MAX_LEN] = '\0';
  nextSlot_ = (nextSlot_ + 1) % ID_HISTORY;
}
