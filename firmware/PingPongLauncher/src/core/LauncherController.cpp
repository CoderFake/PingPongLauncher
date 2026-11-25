#include "LauncherController.h"

#include "../config/AppConfig.h"

void LauncherController::begin() {
  top_.begin(AppConfig::PIN_FLYWHEEL_TOP, AppConfig::CH_FLYWHEEL_TOP);
  bottom_.begin(AppConfig::PIN_FLYWHEEL_BOTTOM, AppConfig::CH_FLYWHEEL_BOTTOM);
  feeder_.begin(AppConfig::PIN_SERVO_FEED, AppConfig::CH_SERVO_FEED);
  turret_.begin(AppConfig::PIN_SERVO_TURRET, AppConfig::CH_SERVO_TURRET);
  if (AppConfig::FLYWHEEL_SENSE_ENABLED) {
    topSense_.begin(AppConfig::PIN_FLYWHEEL_TOP_SENSE);
    bottomSense_.begin(AppConfig::PIN_FLYWHEEL_BOTTOM_SENSE);
  }
  randomSeed(esp_random());
  telemetry_.bootId = esp_random();
}

void LauncherController::apply(const LauncherState& state) {
  power_ = state.power;
  wheelsRequested_ = state.power && state.speedTop > 0 && state.speedBottom > 0;

  uint8_t topTarget = state.power ? state.speedTop : 0;
  uint8_t bottomTarget = state.power ? state.speedBottom : 0;
  if (topTarget != lastTopTarget_ || bottomTarget != lastBottomTarget_) {
    settling_ = false;
    lastTopTarget_ = topTarget;
    lastBottomTarget_ = bottomTarget;
  }
  top_.setTarget(topTarget);
  bottom_.setTarget(bottomTarget);
  turret_.configure(state.turretMode, state.turretRange, state.turretAngle);
  turret_.setActive(state.power);
  feeder_.setRate(state.feedRate);
}

void LauncherController::stopNow() {
  power_ = false;
  wheelsRequested_ = false;
  top_.stopNow();
  bottom_.stopNow();
  feeder_.setEnabled(false);
  turret_.setActive(false);
}

bool LauncherController::update(uint32_t nowMs) {
  top_.update(nowMs);
  bottom_.update(nowMs);
  turret_.update(nowMs);

  uint8_t fault = 0;
  if (AppConfig::FLYWHEEL_SENSE_ENABLED) {
    topSense_.update(nowMs);
    bottomSense_.update(nowMs);
    if (wheelsRequested_ && !topSense_.present(nowMs)) {
      fault |= FAULT_TOP;
    }
    if (wheelsRequested_ && !bottomSense_.present(nowMs)) {
      fault |= FAULT_BOTTOM;
    }
  }

  bool wheelsReady = false;
  if (wheelsRequested_ && fault == 0 && top_.atTarget() && bottom_.atTarget()) {
    if (!settling_) {
      settling_ = true;
      settledSinceMs_ = nowMs;
    }
    wheelsReady = nowMs - settledSinceMs_ >= AppConfig::FLYWHEEL_SETTLE_MS;
  } else {
    settling_ = false;
  }

  bool ready = power_ && wheelsReady;
  feeder_.setEnabled(ready);

  bool changed = false;
  if (fault != telemetry_.wheelFault) {
    telemetry_.wheelFault = fault;
    changed = true;
    if (fault != 0) {
      Serial.printf("[Launcher] Feeding blocked, no PWM on%s%s\n", (fault & FAULT_TOP) ? " top" : "",
                    (fault & FAULT_BOTTOM) ? " bottom" : "");
    } else {
      Serial.println("[Launcher] Flywheel PWM lines OK");
    }
  }
  if (ready != telemetry_.ready) {
    telemetry_.ready = ready;
    changed = true;
  }
  if (feeder_.update(nowMs)) {
    telemetry_.shots++;
    turret_.onShot();
    changed = true;
  }
  return changed;
}
