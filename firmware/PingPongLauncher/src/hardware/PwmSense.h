#pragma once

#include <Arduino.h>

/**
 * Reads back a flywheel PWM line taken from the AOD4184 module's PWM terminal.
 * When the jumper between the ESP32 and the module is unplugged, the read-back
 * pin stays LOW (internal pull-down) and the signal is reported as missing.
 */
class PwmSense {
 public:
  /**
   * Configures the pin with a pull-down and counts rising edges in an interrupt.
   */
  void begin(uint8_t pin);

  /**
   * Samples the line; call every loop.
   */
  void update(uint32_t nowMs);

  /**
   * Returns true when a pulse or a high level was seen within FLYWHEEL_SENSE_WINDOW_MS.
   */
  bool present(uint32_t nowMs) const;

 private:
  static void IRAM_ATTR onEdge(void* arg);

  uint8_t pin_ = 0;
  volatile uint32_t edges_ = 0;
  uint32_t seenEdges_ = 0;
  uint32_t lastSeenMs_ = 0;
  bool seen_ = false;
};
