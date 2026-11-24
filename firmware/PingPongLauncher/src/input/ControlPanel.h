#pragma once

#include <Arduino.h>

#include "../core/LauncherState.h"
#include "Button.h"

/**
 * Setting currently shown on the LCD; Home is the status screen.
 */
enum class PanelItem : uint8_t { Home, SpeedTop, SpeedBottom, SpeedBoth, TurretMode, TurretValue, FeedRate };

/**
 * Sound the panel asks for after handling input.
 */
enum class PanelSound : uint8_t { None, Click, Limit };

/**
 * Outcome of one panel update.
 */
struct PanelResult {
  bool changed = false;
  PanelSound sound = PanelSound::None;
};

/**
 * Four physical buttons: POWER toggles firing, MENU walks through the settings
 * (hold to return home), MINUS and PLUS change the shown setting (hold to repeat).
 * Edits go straight into LauncherState and bump its revision, so they sync to
 * every client like a web command.
 */
class ControlPanel {
 public:
  /**
   * Configures the button pins.
   */
  void begin();

  /**
   * Reads the buttons and applies any edit to `state`.
   */
  PanelResult update(uint32_t nowMs, LauncherState& state);

  /**
   * Returns the setting being edited, or Home.
   */
  PanelItem item() const { return item_; }

 private:
  bool adjust(LauncherState& state, int direction, uint8_t step);
  PanelResult handleStep(LauncherState& state, ButtonEvent event, int direction, uint32_t heldMs);

  Button power_;
  Button menu_;
  Button minus_;
  Button plus_;
  PanelItem item_ = PanelItem::Home;
  uint32_t lastInputMs_ = 0;
};
