#pragma once

#include <Arduino.h>

#include "../hardware/Feeder.h"
#include "../hardware/Flywheel.h"
#include "../hardware/PwmSense.h"
#include "../hardware/Turret.h"
#include "LauncherState.h"

/**
 * Drives flywheels, turret and feeder from the current LauncherState.
 * Feeding only starts when both flywheels are requested, their PWM lines are
 * confirmed by the read-back pins, and both reached speed and settled.
 */
class LauncherController {
 public:
  /**
   * Initializes all actuators in a safe, stopped position.
   */
  void begin();

  /**
   * Pushes new settings to the actuators.
   */
  void apply(const LauncherState& state);

  /**
   * Stops both flywheels instantly before blocking work such as flashing.
   */
  void stopNow();

  /**
   * Runs one control step; returns true when telemetry changed.
   */
  bool update(uint32_t nowMs);

  /**
   * Bit flags for wheel faults in LauncherTelemetry::wheelFault.
   */
  static constexpr uint8_t FAULT_TOP = 1;
  static constexpr uint8_t FAULT_BOTTOM = 2;

  /**
   * Returns the latest telemetry snapshot.
   */
  const LauncherTelemetry& telemetry() const { return telemetry_; }

 private:
  Flywheel top_;
  Flywheel bottom_;
  Turret turret_;
  Feeder feeder_;
  PwmSense topSense_;
  PwmSense bottomSense_;
  LauncherTelemetry telemetry_;
  bool power_ = false;
  bool wheelsRequested_ = false;
  uint32_t settledSinceMs_ = 0;
  bool settling_ = false;
  uint8_t lastTopTarget_ = 0;
  uint8_t lastBottomTarget_ = 0;
};
