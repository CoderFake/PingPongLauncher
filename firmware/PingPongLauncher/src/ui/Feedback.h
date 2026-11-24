#pragma once

#include <Arduino.h>

#include "../core/LauncherState.h"
#include "../input/ControlPanel.h"
#include "../net/WifiController.h"
#include "Buzzer.h"
#include "Lcd1602.h"

/**
 * Everything the LCD and buzzer react to, sampled from the main loop.
 */
struct SystemSnapshot {
  WifiStatus wifi;
  bool mqtt = false;
  LauncherState state;
  LauncherTelemetry telemetry;
  uint32_t resetHeldMs = 0;
  PanelItem panel = PanelItem::Home;
};

/**
 * User-facing status output: English screens on the optional LCD and
 * short beeps on the buzzer whenever the setup or run state changes.
 */
class Feedback {
 public:
  /**
   * Starts both outputs and shows the boot splash.
   */
  void begin(const String& deviceId, uint32_t nowMs);

  /**
   * Compares with the previous snapshot, beeps on transitions and redraws.
   */
  void update(const SystemSnapshot& snapshot, uint32_t nowMs);

  /**
   * Advances the melody and LCD writes; call every loop.
   */
  void tick(uint32_t nowMs);

  /**
   * Reports OTA progress; refreshes the LCD immediately because OTA blocks the loop.
   */
  void onOta(const char* state, int progress, const char* error);

  /**
   * Shows the reset screen and plays the confirmation before a reboot.
   */
  void onResetConfirmed();

  /**
   * Plays the button click or limit sound requested by the control panel.
   */
  void onPanel(PanelSound sound);

 private:
  void announce(const SystemSnapshot& now);
  void compose(const SystemSnapshot& snapshot, uint32_t nowMs, char* line1, char* line2);
  void composeRunning(const SystemSnapshot& snapshot, bool pageB, char* line1, char* line2);
  void composeEdit(const SystemSnapshot& snapshot, char* line1, char* line2);
  void composeOta(char* line1, char* line2);
  static const char* wifiErrorText(const String& code);

  Lcd1602 lcd_;
  Buzzer buzzer_;
  String deviceId_;
  SystemSnapshot previous_;
  uint32_t bootUntilMs_ = 0;
  char otaState_[12] = "";
  char otaError_[20] = "";
  int otaProgress_ = 0;
  uint32_t otaShownAtMs_ = 0;
};
