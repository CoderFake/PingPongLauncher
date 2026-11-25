#include "PwmSense.h"

#include "../config/AppConfig.h"

void PwmSense::begin(uint8_t pin) {
  pin_ = pin;
  pinMode(pin_, INPUT_PULLDOWN);
  attachInterruptArg(digitalPinToInterrupt(pin_), onEdge, this, RISING);
}

void PwmSense::update(uint32_t nowMs) {
  uint32_t edges = edges_;
  if (edges != seenEdges_ || digitalRead(pin_) == HIGH) {
    seenEdges_ = edges;
    lastSeenMs_ = nowMs;
    seen_ = true;
  }
}

bool PwmSense::present(uint32_t nowMs) const {
  return seen_ && nowMs - lastSeenMs_ < AppConfig::FLYWHEEL_SENSE_WINDOW_MS;
}

void IRAM_ATTR PwmSense::onEdge(void* arg) {
  PwmSense* self = static_cast<PwmSense*>(arg);
  self->edges_ = self->edges_ + 1;
}
