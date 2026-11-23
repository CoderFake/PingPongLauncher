#pragma once

#include <Arduino.h>

/**
 * Software rollback for freshly installed firmware.
 * After an OTA the new image runs on trial; if it reboots too many times
 * before reaching MQTT, the previous slot is restored.
 */
class OtaGuard {
 public:
  /**
   * Counts a trial boot and rolls back when the limit is exceeded.
   * Call first thing in setup().
   */
  void begin();

  /**
   * Starts a trial for the image that will boot next.
   */
  void armTrial();

  /**
   * Confirms the running image works; ends any trial.
   */
  void markHealthy();

 private:
  bool trial_ = false;
};
