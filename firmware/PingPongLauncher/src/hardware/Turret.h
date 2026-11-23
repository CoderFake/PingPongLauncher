#pragma once

#include <Arduino.h>

#include "../core/LauncherState.h"
#include "ServoDriver.h"

/**
 * Pan servo that aims the barrel. Angles are relative to center:
 * negative is left, positive is right, limited to ±TURRET_LIMIT_DEG.
 */
class Turret {
 public:
  /**
   * Attaches the servo and centers the barrel.
   */
  void begin(uint8_t pin, uint8_t channel);

  /**
   * Applies mode, sweep/random range and fixed angle from the state.
   */
  void configure(TurretMode mode, uint8_t range, int8_t fixedAngle);

  /**
   * Enables motion; when disabled the barrel returns to center.
   */
  void setActive(bool active);

  /**
   * Picks a new random aim; call after each fired ball.
   */
  void onShot();

  /**
   * Moves toward the current target with a speed limit; call every loop.
   */
  void update(uint32_t nowMs);

 private:
  void pickRandomTarget();
  void writeRelative(float relativeDeg);

  ServoDriver servo_;
  TurretMode mode_ = TurretMode::Sweep;
  float range_ = 25.0f;
  float fixedAngle_ = 0.0f;
  bool active_ = false;
  float position_ = 0.0f;
  float target_ = 0.0f;
  int8_t sweepDirection_ = 1;
  uint32_t lastUpdateMs_ = 0;
};
