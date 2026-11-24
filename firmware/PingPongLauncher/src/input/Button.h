#pragma once

#include <Arduino.h>

/**
 * Events produced by a debounced push button.
 * Press fires on the debounced down edge, Repeat while held after a delay,
 * LongPress once when the hold reaches BUTTON_LONG_MS.
 */
enum class ButtonEvent : uint8_t { None, Press, Repeat, LongPress };

/**
 * Momentary push button wired between the pin and GND (internal pull-up).
 */
class Button {
 public:
  /**
   * Configures the pin with the internal pull-up.
   */
  void begin(uint8_t pin);

  /**
   * Samples the pin and returns at most one event per call.
   */
  ButtonEvent update(uint32_t nowMs);

  /**
   * Returns how long the button has been held, or 0 when released.
   */
  uint32_t heldMs(uint32_t nowMs) const { return down_ ? nowMs - downSinceMs_ : 0; }

 private:
  uint8_t pin_ = 0;
  bool raw_ = false;
  bool down_ = false;
  bool longFired_ = false;
  uint32_t rawSinceMs_ = 0;
  uint32_t downSinceMs_ = 0;
  uint32_t lastRepeatMs_ = 0;
};
