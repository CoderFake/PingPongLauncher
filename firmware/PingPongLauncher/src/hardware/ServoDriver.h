#pragma once

#include <Arduino.h>

/**
 * Minimal hobby-servo driver on a dedicated 50 Hz LEDC channel.
 */
class ServoDriver {
 public:
  /**
   * Attaches the signal pin to a fixed LEDC channel.
   */
  void begin(uint8_t pin, uint8_t channel);

  /**
   * Moves the horn to an absolute angle in degrees (0-180).
   */
  void write(float degrees);

  /**
   * Returns the last commanded angle.
   */
  float angle() const { return angle_; }

 private:
  uint8_t pin_ = 0;
  float angle_ = -1.0f;
};
