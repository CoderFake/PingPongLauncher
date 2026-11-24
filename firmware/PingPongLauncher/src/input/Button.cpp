#include "Button.h"

#include "../config/AppConfig.h"

void Button::begin(uint8_t pin) {
  pin_ = pin;
  pinMode(pin_, INPUT_PULLUP);
}

ButtonEvent Button::update(uint32_t nowMs) {
  bool raw = digitalRead(pin_) == LOW;
  if (raw != raw_) {
    raw_ = raw;
    rawSinceMs_ = nowMs;
  }
  if (nowMs - rawSinceMs_ < AppConfig::BUTTON_DEBOUNCE_MS) {
    return ButtonEvent::None;
  }

  if (raw_ && !down_) {
    down_ = true;
    longFired_ = false;
    downSinceMs_ = nowMs;
    lastRepeatMs_ = nowMs;
    return ButtonEvent::Press;
  }
  if (!raw_) {
    down_ = false;
    return ButtonEvent::None;
  }

  uint32_t held = nowMs - downSinceMs_;
  if (!longFired_ && held >= AppConfig::BUTTON_LONG_MS) {
    longFired_ = true;
    return ButtonEvent::LongPress;
  }
  if (held >= AppConfig::BUTTON_REPEAT_DELAY_MS && nowMs - lastRepeatMs_ >= AppConfig::BUTTON_REPEAT_MS) {
    lastRepeatMs_ = nowMs;
    return ButtonEvent::Repeat;
  }
  return ButtonEvent::None;
}
