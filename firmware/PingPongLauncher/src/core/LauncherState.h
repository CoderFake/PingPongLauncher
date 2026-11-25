#pragma once

#include <Arduino.h>

#include "../config/AppConfig.h"

/**
 * Turret motion modes selectable from the web UI.
 */
enum class TurretMode : uint8_t { Sweep, Random, Fixed };

/**
 * User-controlled launcher settings, synchronized over MQTT.
 * `rev` increases on every accepted change so clients can drop stale state.
 */
struct LauncherState {
  bool power = false;
  uint8_t speedTop = AppConfig::DEFAULT_SPEED_TOP;
  uint8_t speedBottom = AppConfig::DEFAULT_SPEED_BOTTOM;
  TurretMode turretMode = TurretMode::Sweep;
  uint8_t turretRange = AppConfig::DEFAULT_TURRET_RANGE;
  int8_t turretAngle = 0;
  uint8_t feedRate = AppConfig::DEFAULT_FEED_RATE;
  uint32_t rev = 1;
};

/**
 * Runtime values reported by the device, never written by clients.
 */
struct LauncherTelemetry {
  bool ready = false;
  uint8_t wheelFault = 0;
  uint32_t shots = 0;
  uint32_t bootId = 0;
};
