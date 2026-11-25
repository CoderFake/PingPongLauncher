#include "Turret.h"

#include "../config/AppConfig.h"

void Turret::begin(uint8_t pin, uint8_t channel) {
  servo_.begin(pin, channel);
  position_ = 0.0f;
  target_ = 0.0f;
  writeRelative(position_);
  lastUpdateMs_ = millis();
}

void Turret::configure(TurretMode mode, uint8_t range, int8_t fixedAngle) {
  bool modeChanged = mode != mode_;
  mode_ = mode;
  range_ = constrain(static_cast<float>(range), static_cast<float>(AppConfig::TURRET_RANGE_MIN),
                     static_cast<float>(AppConfig::TURRET_LIMIT_DEG));
  fixedAngle_ = constrain(static_cast<float>(fixedAngle),
                          -static_cast<float>(AppConfig::TURRET_LIMIT_DEG),
                          static_cast<float>(AppConfig::TURRET_LIMIT_DEG));
  if (modeChanged && mode_ == TurretMode::Random) {
    pickRandomTarget();
  }
}

void Turret::setActive(bool active) {
  bool starting = active && !active_;
  active_ = active;
  if (starting && mode_ == TurretMode::Random) {
    pickRandomTarget();
  }
}

void Turret::onShot() {
  if (active_ && mode_ == TurretMode::Random) {
    pickRandomTarget();
  }
}

void Turret::update(uint32_t nowMs) {
  float dt = (nowMs - lastUpdateMs_) / 1000.0f;
  lastUpdateMs_ = nowMs;
  if (dt <= 0.0f) {
    return;
  }
  dt = min(dt, 0.05f);

  if (!active_) {
    target_ = 0.0f;
  } else if (mode_ == TurretMode::Fixed) {
    target_ = fixedAngle_;
  } else if (mode_ == TurretMode::Sweep) {
    if (fabsf(position_) > range_ + 0.5f) {
      target_ = constrain(position_, -range_, range_);
    } else {
      target_ = position_ + sweepDirection_ * AppConfig::TURRET_SWEEP_DEG_PER_SEC * dt;
      if (target_ >= range_) {
        target_ = range_;
        sweepDirection_ = -1;
      } else if (target_ <= -range_) {
        target_ = -range_;
        sweepDirection_ = 1;
      }
    }
  } else if (fabsf(target_) > range_ || nowMs - lastPickMs_ >= AppConfig::TURRET_RANDOM_HOLD_MS) {
    pickRandomTarget();
  }

  float maxStep = AppConfig::TURRET_MAX_DEG_PER_SEC * dt;
  float delta = target_ - position_;
  position_ += constrain(delta, -maxStep, maxStep);
  writeRelative(position_);
}

void Turret::pickRandomTarget() {
  int span = static_cast<int>(range_);
  float minJump = range_ * 0.4f;
  float next = position_;
  for (uint8_t attempt = 0; attempt < 6 && fabsf(next - position_) < minJump; attempt++) {
    next = static_cast<float>(random(-span, span + 1));
  }
  target_ = next;
  lastPickMs_ = millis();
}

void Turret::writeRelative(float relativeDeg) {
  float signedDeg = AppConfig::TURRET_INVERT ? -relativeDeg : relativeDeg;
  servo_.write(AppConfig::TURRET_CENTER_DEG + signedDeg);
}
