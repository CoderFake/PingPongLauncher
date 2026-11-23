#pragma once

#include <Arduino.h>

/**
 * One ZWF-30 flywheel driven through an AOD4184 MOSFET module.
 * Speed changes are ramped to avoid inrush current dips on the 12V rail.
 */
class Flywheel {
 public:
  /**
   * Attaches the PWM pin to a fixed LEDC channel and stops the motor.
   */
  void begin(uint8_t pin, uint8_t channel);

  /**
   * Sets the target speed in percent (0-100).
   */
  void setTarget(uint8_t percent);

  /**
   * Cuts the output immediately, skipping the ramp.
   */
  void stopNow();

  /**
   * Advances the ramp toward the target; call every loop.
   */
  void update(uint32_t nowMs);

  /**
   * Returns true when the output has reached the target speed.
   */
  bool atTarget() const { return current_ == target_; }

  /**
   * Returns the current output in percent.
   */
  uint8_t current() const { return current_; }

 private:
  void writeOutput();

  uint8_t pin_ = 0;
  uint8_t target_ = 0;
  uint8_t current_ = 0;
  uint32_t lastStepMs_ = 0;
};
