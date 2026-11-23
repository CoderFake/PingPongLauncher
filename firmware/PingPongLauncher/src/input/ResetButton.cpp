#include "ResetButton.h"

void ResetButton::begin(uint8_t pin, uint32_t holdMs, Callback onLongPress) {
  pin_ = pin;
  holdMs_ = holdMs;
  onLongPress_ = onLongPress;
  pinMode(pin_, INPUT_PULLUP);
}

void ResetButton::update(uint32_t nowMs) {
  bool down = digitalRead(pin_) == LOW;
  if (!down) {
    pressed_ = false;
    fired_ = false;
    return;
  }
  if (!pressed_) {
    pressed_ = true;
    pressedSinceMs_ = nowMs;
    return;
  }
  if (!fired_ && nowMs - pressedSinceMs_ >= holdMs_) {
    fired_ = true;
    if (onLongPress_) {
      onLongPress_();
    }
  }
}
