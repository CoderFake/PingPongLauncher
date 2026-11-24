#include "ControlPanel.h"

#include "../config/AppConfig.h"

namespace {

constexpr uint8_t ITEM_COUNT = 7;

/**
 * Moves `value` by `delta` inside [minValue, maxValue]; returns false when it could not move.
 */
template <typename T>
bool stepValue(T& value, int delta, int minValue, int maxValue) {
  int next = constrain(static_cast<int>(value) + delta, minValue, maxValue);
  if (next == static_cast<int>(value)) {
    return false;
  }
  value = static_cast<T>(next);
  return true;
}

/**
 * Cycles Sweep, Random, Fixed in the given direction.
 */
TurretMode nextMode(TurretMode mode, int direction) {
  int index = (static_cast<int>(mode) + direction + 3) % 3;
  return static_cast<TurretMode>(index);
}

}

void ControlPanel::begin() {
  power_.begin(AppConfig::PIN_BTN_POWER);
  menu_.begin(AppConfig::PIN_BTN_MENU);
  minus_.begin(AppConfig::PIN_BTN_MINUS);
  plus_.begin(AppConfig::PIN_BTN_PLUS);
}

PanelResult ControlPanel::update(uint32_t nowMs, LauncherState& state) {
  PanelResult result;

  ButtonEvent powerEvent = power_.update(nowMs);
  ButtonEvent menuEvent = menu_.update(nowMs);
  ButtonEvent minusEvent = minus_.update(nowMs);
  ButtonEvent plusEvent = plus_.update(nowMs);

  if (powerEvent == ButtonEvent::Press) {
    lastInputMs_ = nowMs;
    state.power = !state.power;
    state.rev++;
    result.changed = true;
    Serial.printf("[Panel] Power %s\n", state.power ? "on" : "off");
    return result;
  }

  if (menuEvent == ButtonEvent::Press) {
    lastInputMs_ = nowMs;
    item_ = static_cast<PanelItem>((static_cast<uint8_t>(item_) + 1) % ITEM_COUNT);
    result.sound = PanelSound::Click;
    return result;
  }
  if (menuEvent == ButtonEvent::LongPress) {
    item_ = PanelItem::Home;
    result.sound = PanelSound::Click;
    return result;
  }

  if (minusEvent != ButtonEvent::None) {
    return handleStep(state, minusEvent, -1, minus_.heldMs(nowMs));
  }
  if (plusEvent != ButtonEvent::None) {
    return handleStep(state, plusEvent, 1, plus_.heldMs(nowMs));
  }

  if (item_ != PanelItem::Home && nowMs - lastInputMs_ >= AppConfig::PANEL_IDLE_MS) {
    item_ = PanelItem::Home;
  }
  return result;
}

PanelResult ControlPanel::handleStep(LauncherState& state, ButtonEvent event, int direction,
                                     uint32_t heldMs) {
  PanelResult result;
  if (item_ == PanelItem::Home || event == ButtonEvent::LongPress) {
    return result;
  }
  bool repeat = event == ButtonEvent::Repeat;
  if (repeat && item_ == PanelItem::TurretMode) {
    return result;
  }

  lastInputMs_ = millis();
  uint8_t step = repeat && heldMs >= AppConfig::PANEL_FAST_AFTER_MS ? AppConfig::PANEL_FAST_STEP : 1;
  if (adjust(state, direction, step)) {
    state.rev++;
    result.changed = true;
    if (!repeat) {
      result.sound = PanelSound::Click;
    }
  } else if (!repeat) {
    result.sound = PanelSound::Limit;
  }
  return result;
}

bool ControlPanel::adjust(LauncherState& state, int direction, uint8_t step) {
  int delta = direction * step;
  switch (item_) {
    case PanelItem::SpeedTop:
      return stepValue(state.speedTop, delta, 0, 100);
    case PanelItem::SpeedBottom:
      return stepValue(state.speedBottom, delta, 0, 100);
    case PanelItem::SpeedBoth: {
      uint8_t both = (state.speedTop + state.speedBottom + 1) / 2;
      bool moved = stepValue(both, delta, 0, 100);
      bool differs = state.speedTop != both || state.speedBottom != both;
      state.speedTop = both;
      state.speedBottom = both;
      return moved || differs;
    }
    case PanelItem::TurretMode:
      state.turretMode = nextMode(state.turretMode, direction);
      return true;
    case PanelItem::TurretValue:
      if (state.turretMode == TurretMode::Fixed) {
        return stepValue(state.turretAngle, delta, -AppConfig::TURRET_LIMIT_DEG, AppConfig::TURRET_LIMIT_DEG);
      }
      return stepValue(state.turretRange, delta, AppConfig::TURRET_RANGE_MIN, AppConfig::TURRET_LIMIT_DEG);
    case PanelItem::FeedRate:
      return stepValue(state.feedRate, delta, AppConfig::FEED_RATE_MIN, AppConfig::FEED_RATE_MAX);
    default:
      return false;
  }
}
