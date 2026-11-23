#pragma once

#include <Arduino.h>

#include "../ota/OtaUpdater.h"
#include "LauncherState.h"

/**
 * Outcome of applying one MQTT command payload.
 */
enum class CommandResult : uint8_t {
  Invalid,
  Duplicate,
  NoChange,
  Changed,
  StateRequested,
  OtaRequested
};

/**
 * Parses client commands into LauncherState and serializes state to JSON.
 * Remembers recent command ids so retransmitted messages are applied once.
 */
class StateCodec {
 public:
  /**
   * Applies a JSON command such as {"id":"k3x9","sb1":75} to `state`.
   * An {"ota":{url,sha256,version}} command fills `ota` instead.
   */
  CommandResult applyCommand(const uint8_t* payload, size_t length, LauncherState& state,
                             OtaRequest& ota);

  /**
   * Writes the full state plus telemetry as a JSON object into `out`.
   */
  size_t serialize(const LauncherState& state, const LauncherTelemetry& telemetry, char* out,
                   size_t capacity) const;

  /**
   * Converts a mode to its wire name.
   */
  static const char* modeToString(TurretMode mode);

  /**
   * Parses a wire name into a mode; returns false for unknown names.
   */
  static bool modeFromString(const char* text, TurretMode& mode);

 private:
  static constexpr uint8_t ID_HISTORY = 24;
  static constexpr uint8_t ID_MAX_LEN = 24;

  bool seenRecently(const char* id) const;
  void remember(const char* id);

  char recentIds_[ID_HISTORY][ID_MAX_LEN + 1] = {};
  uint8_t nextSlot_ = 0;
};
