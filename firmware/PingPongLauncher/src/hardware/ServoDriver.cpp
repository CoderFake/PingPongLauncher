#include "ServoDriver.h"

#include "../config/AppConfig.h"

void ServoDriver::begin(uint8_t pin, uint8_t channel) {
  pin_ = pin;
  ledcAttachChannel(pin_, AppConfig::SERVO_PWM_FREQ, AppConfig::SERVO_PWM_BITS, channel);
}

void ServoDriver::write(float degrees) {
  degrees = constrain(degrees, 0.0f, 180.0f);
  if (fabsf(degrees - angle_) < 0.1f) {
    return;
  }
  angle_ = degrees;

  const float pulseUs = AppConfig::SERVO_MIN_US +
                        (AppConfig::SERVO_MAX_US - AppConfig::SERVO_MIN_US) * degrees / 180.0f;
  const uint32_t periodUs = 1000000UL / AppConfig::SERVO_PWM_FREQ;
  const uint32_t maxDuty = (1UL << AppConfig::SERVO_PWM_BITS) - 1;
  ledcWrite(pin_, static_cast<uint32_t>(pulseUs * maxDuty / periodUs));
}
