#include "Feeder.h"

#include "../config/AppConfig.h"

namespace {

/**
 * Time the servo needs to sweep from rest to the lift angle at its rated speed.
 */
constexpr uint32_t travelMs() {
  return static_cast<uint32_t>((AppConfig::FEED_UP_DEG - AppConfig::FEED_DOWN_DEG) *
                               AppConfig::SERVO_MS_PER_60DEG / 60.0f);
}

constexpr uint32_t LIFT_MS = travelMs() + AppConfig::FEED_HOLD_UP_MS;
constexpr uint32_t RETURN_MS = travelMs() + AppConfig::FEED_SETTLE_DOWN_MS;

}

void Feeder::begin(uint8_t pin, uint8_t channel) {
  servo_.begin(pin, channel);
  servo_.write(AppConfig::FEED_DOWN_DEG);
  phase_ = Phase::Idle;
}

void Feeder::setRate(uint8_t ballsPerMinute) {
  rate_ = constrain(ballsPerMinute, AppConfig::FEED_RATE_MIN, AppConfig::FEED_RATE_MAX);
}

void Feeder::setEnabled(bool enabled) {
  if (enabled && !enabled_) {
    cycleStartMs_ = millis() - periodMs();
  }
  enabled_ = enabled;
}

bool Feeder::update(uint32_t nowMs) {
  switch (phase_) {
    case Phase::Idle:
      if (enabled_ && nowMs - cycleStartMs_ >= periodMs()) {
        cycleStartMs_ = nowMs;
        phaseStartMs_ = nowMs;
        phase_ = Phase::Lifting;
        servo_.write(AppConfig::FEED_UP_DEG);
        return true;
      }
      break;
    case Phase::Lifting:
      if (nowMs - phaseStartMs_ >= LIFT_MS) {
        phaseStartMs_ = nowMs;
        phase_ = Phase::Returning;
        servo_.write(AppConfig::FEED_DOWN_DEG);
      }
      break;
    case Phase::Returning:
      if (nowMs - phaseStartMs_ >= RETURN_MS) {
        phase_ = Phase::Idle;
      }
      break;
  }
  return false;
}

uint32_t Feeder::periodMs() const {
  uint32_t period = 60000UL / rate_;
  uint32_t minimum = LIFT_MS + RETURN_MS;
  return period > minimum ? period : minimum;
}
