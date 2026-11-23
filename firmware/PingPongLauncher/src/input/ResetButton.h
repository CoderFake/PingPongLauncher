#pragma once

#include <Arduino.h>

#include <functional>

/**
 * Detects a long press on the BOOT button and fires a callback once.
 */
class ResetButton {
 public:
  using Callback = std::function<void()>;

  /**
   * Configures the pin with a pull-up and stores the callback.
   */
  void begin(uint8_t pin, uint32_t holdMs, Callback onLongPress);

  /**
   * Samples the button; call every loop.
   */
  void update(uint32_t nowMs);

 private:
  uint8_t pin_ = 0;
  uint32_t holdMs_ = 0;
  uint32_t pressedSinceMs_ = 0;
  bool pressed_ = false;
  bool fired_ = false;
  Callback onLongPress_;
};
