#pragma once

#include <Arduino.h>

/**
 * Optional 16x2 HD44780 LCD behind a PCF8574 I2C backpack.
 * The display is probed periodically, so it can be plugged in or pulled
 * out at runtime; every call is a no-op while it is absent.
 */
class Lcd1602 {
 public:
  static constexpr uint8_t COLS = 16;

  /**
   * Starts the I2C bus and tries to find the display.
   */
  void begin(uint32_t nowMs);

  /**
   * Re-probes when absent; call every loop.
   */
  void update(uint32_t nowMs);

  /**
   * Shows two lines, writing only the lines that changed.
   */
  void show(const char* line1, const char* line2);

  /**
   * Returns true while a display answers on the bus.
   */
  bool present() const { return address_ != 0; }

 private:
  bool probe();
  bool init();
  bool writeLine(uint8_t row, const char* text);
  bool sendNibble(uint8_t nibble, bool data);
  bool sendCommand(uint8_t command);
  void markLost();

  uint8_t address_ = 0;
  uint32_t lastProbeMs_ = 0;
  char shown_[2][COLS + 1] = {};
  char wanted_[2][COLS + 1] = {};
};
