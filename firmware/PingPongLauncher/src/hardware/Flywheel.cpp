#include "Flywheel.h"

#include "../config/AppConfig.h"

void Flywheel::begin(uint8_t pin, uint8_t channel) {
  pin_ = pin;
  ledcAttachChannel(pin_, AppConfig::FLYWHEEL_PWM_FREQ, AppConfig::FLYWHEEL_PWM_BITS, channel);
  current_ = 0;
  target_ = 0;
  writeOutput();
}

void Flywheel::setTarget(uint8_t percent) {
  target_ = percent > 100 ? 100 : percent;
}

void Flywheel::stopNow() {
  target_ = 0;
  current_ = 0;
  writeOutput();
}

void Flywheel::update(uint32_t nowMs) {
  if (current_ == target_ || nowMs - lastStepMs_ < AppConfig::FLYWHEEL_RAMP_INTERVAL_MS) {
    return;
  }
  lastStepMs_ = nowMs;

  int step = AppConfig::FLYWHEEL_RAMP_STEP;
  int delta = static_cast<int>(target_) - static_cast<int>(current_);
  if (abs(delta) <= step) {
    current_ = target_;
  } else {
    current_ += delta > 0 ? step : -step;
  }
  writeOutput();
}

void Flywheel::writeOutput() {
  const uint32_t maxDuty = (1UL << AppConfig::FLYWHEEL_PWM_BITS) - 1;
  ledcWrite(pin_, maxDuty * current_ / 100);
}
