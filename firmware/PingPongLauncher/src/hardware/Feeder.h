#pragma once

#include <Arduino.h>

#include "ServoDriver.h"

/**
 * Piston servo that lifts the ball column one ball per stroke,
 * pushing the top ball into the flywheels.
 */
class Feeder {
 public:
  /**
   * Attaches the servo and parks the piston at the bottom.
   */
  void begin(uint8_t pin, uint8_t channel);

  /**
   * Sets the feed rate in balls per minute.
   */
  void setRate(uint8_t ballsPerMinute);

  /**
   * Starts or stops feeding; a running stroke always completes.
   */
  void setEnabled(bool enabled);

  /**
   * Runs the stroke state machine; returns true when a ball was pushed.
   */
  bool update(uint32_t nowMs);

 private:
  enum class Phase : uint8_t { Idle, Lifting, Returning };

  uint32_t periodMs() const;

  ServoDriver servo_;
  Phase phase_ = Phase::Idle;
  uint8_t rate_ = 30;
  bool enabled_ = false;
  uint32_t phaseStartMs_ = 0;
  uint32_t cycleStartMs_ = 0;
};
