#include "Feedback.h"

#include "../config/AppConfig.h"

namespace {

constexpr uint32_t BOOT_SPLASH_MS = 1500;
constexpr uint32_t OTA_ERROR_SHOW_MS = 6000;
constexpr uint32_t RESET_HINT_AFTER_MS = 300;
constexpr char DEGREE = static_cast<char>(0xDF);

/**
 * Formats a turret description such as "Sweep +-25" or "Fixed +12°".
 */
void turretText(const LauncherState& state, char* out, size_t size) {
  switch (state.turretMode) {
    case TurretMode::Fixed:
      snprintf(out, size, "Fixed %+d%c", state.turretAngle, DEGREE);
      break;
    case TurretMode::Random:
      snprintf(out, size, "Rand +-%u", state.turretRange);
      break;
    default:
      snprintf(out, size, "Sweep +-%u", state.turretRange);
      break;
  }
}

}

void Feedback::begin(const String& deviceId, uint32_t nowMs) {
  deviceId_ = deviceId;
  buzzer_.begin();
  lcd_.begin(nowMs);
  bootUntilMs_ = nowMs + BOOT_SPLASH_MS;
  previous_.wifi.state = WifiState::Connecting;

  char line2[24];
  snprintf(line2, sizeof(line2), "v%-8s%s", FIRMWARE_VERSION, deviceId_.c_str());
  lcd_.show("PingPong Launch", line2);
  lcd_.update(nowMs);
  buzzer_.play(Sound::Boot);
}

void Feedback::tick(uint32_t nowMs) {
  buzzer_.update(nowMs);
  lcd_.update(nowMs);
}

void Feedback::update(const SystemSnapshot& snapshot, uint32_t nowMs) {
  if (nowMs >= bootUntilMs_) {
    announce(snapshot);
    previous_ = snapshot;
  }

  char line1[24];
  char line2[24];
  compose(snapshot, nowMs, line1, line2);
  lcd_.show(line1, line2);
}

void Feedback::onOta(const char* state, int progress, const char* error) {
  bool starting = strcmp(state, "downloading") == 0 && strcmp(otaState_, "downloading") != 0;
  strncpy(otaState_, state, sizeof(otaState_) - 1);
  strncpy(otaError_, error ? error : "", sizeof(otaError_) - 1);
  otaProgress_ = progress;
  otaShownAtMs_ = millis();

  if (starting) {
    buzzer_.playBlocking(Sound::OtaStart);
  } else if (strcmp(state, "error") == 0) {
    buzzer_.play(Sound::Error);
  }

  char line1[24];
  char line2[24];
  composeOta(line1, line2);
  lcd_.show(line1, line2);
  lcd_.update(millis());

  if (strcmp(state, "rebooting") == 0) {
    buzzer_.playBlocking(Sound::OtaDone);
  }
}

void Feedback::onResetConfirmed() {
  lcd_.show("WiFi reset", "Rebooting...");
  lcd_.update(millis());
  buzzer_.playBlocking(Sound::ResetConfirm);
}

void Feedback::announce(const SystemSnapshot& now) {
  const SystemSnapshot& before = previous_;

  if (now.wifi.state != before.wifi.state) {
    switch (now.wifi.state) {
      case WifiState::Portal:
        buzzer_.play(Sound::SetupNeeded);
        return;
      case WifiState::JoinFailed:
      case WifiState::Lost:
        buzzer_.play(Sound::WifiFailed);
        return;
      case WifiState::Joined:
        buzzer_.play(Sound::WifiJoined);
        return;
      case WifiState::Online:
        if (before.wifi.state != WifiState::Joined) {
          buzzer_.play(Sound::WifiJoined);
          return;
        }
        break;
      default:
        break;
    }
  }

  if (now.mqtt != before.mqtt) {
    buzzer_.play(now.mqtt ? Sound::CloudOnline : Sound::CloudLost);
    return;
  }
  if (now.state.power != before.state.power) {
    buzzer_.play(now.state.power ? Sound::PowerOn : Sound::PowerOff);
    return;
  }
  if (now.telemetry.ready && !before.telemetry.ready) {
    buzzer_.play(Sound::Ready);
    return;
  }
  if (now.resetHeldMs / 1000 > before.resetHeldMs / 1000 && now.resetHeldMs > 0) {
    buzzer_.play(Sound::Tick);
  }
}

void Feedback::compose(const SystemSnapshot& snapshot, uint32_t nowMs, char* line1, char* line2) {
  bool pageB = (nowMs / AppConfig::LCD_PAGE_INTERVAL_MS) % 2 == 1;
  bool otaActive = otaState_[0] != '\0' &&
                   (strcmp(otaState_, "error") != 0 || nowMs - otaShownAtMs_ < OTA_ERROR_SHOW_MS);

  if (otaActive) {
    composeOta(line1, line2);
    return;
  }
  if (snapshot.resetHeldMs > RESET_HINT_AFTER_MS) {
    uint32_t left = snapshot.resetHeldMs >= AppConfig::RESET_HOLD_MS
                        ? 0
                        : (AppConfig::RESET_HOLD_MS - snapshot.resetHeldMs + 999) / 1000;
    strcpy(line1, "Hold to reset");
    snprintf(line2, 24, "WiFi in %lus...", static_cast<unsigned long>(left));
    return;
  }
  if (nowMs < bootUntilMs_) {
    strcpy(line1, "PingPong Launch");
    snprintf(line2, 24, "v%-8s%s", FIRMWARE_VERSION, deviceId_.c_str());
    return;
  }

  const WifiStatus& wifi = snapshot.wifi;
  switch (wifi.state) {
    case WifiState::Connecting:
      strcpy(line1, "Connecting WiFi");
      snprintf(line2, 24, "%s", wifi.ssid.c_str());
      return;
    case WifiState::Joining:
      strcpy(line1, "Joining WiFi...");
      snprintf(line2, 24, "%s", wifi.ssid.c_str());
      return;
    case WifiState::JoinFailed:
      if (!pageB) {
        strcpy(line1, "WiFi failed");
        strcpy(line2, wifiErrorText(wifi.error));
        return;
      }
      [[fallthrough]];
    case WifiState::Portal:
      if (!pageB) {
        strcpy(line1, "Setup: join WiFi");
        snprintf(line2, 24, "%s", wifi.apSsid.c_str());
      } else {
        snprintf(line1, 24, "Pass: %s", AppConfig::AP_PASSWORD);
        strcpy(line2, "Open 192.168.4.1");
      }
      return;
    case WifiState::Joined:
      strcpy(line1, "WiFi connected");
      snprintf(line2, 24, "%s", wifi.ip.c_str());
      return;
    case WifiState::Lost:
      strcpy(line1, "WiFi lost");
      strcpy(line2, "Reconnecting...");
      return;
    case WifiState::Online:
      break;
  }

  if (!snapshot.mqtt) {
    strcpy(line1, "Cloud connecting");
    snprintf(line2, 24, "%s", wifi.ip.c_str());
    return;
  }
  composeRunning(snapshot, pageB, line1, line2);
}

void Feedback::composeRunning(const SystemSnapshot& snapshot, bool pageB, char* line1, char* line2) {
  const LauncherState& state = snapshot.state;
  char turret[16];
  turretText(state, turret, sizeof(turret));

  if (!state.power) {
    snprintf(line1, 24, "OFF    ID %s", deviceId_.c_str());
    if (pageB) {
      snprintf(line2, 24, "%s", snapshot.wifi.ip.c_str());
    } else {
      snprintf(line2, 24, "SB %u/%u %u/m", state.speedTop, state.speedBottom, state.feedRate);
    }
    return;
  }

  if (snapshot.telemetry.ready) {
    snprintf(line1, 24, "FIRING %9lu", static_cast<unsigned long>(snapshot.telemetry.shots));
  } else {
    strcpy(line1, "Spinning up...");
  }
  if (pageB) {
    snprintf(line2, 24, "%s %u/m", turret, state.feedRate);
  } else {
    snprintf(line2, 24, "SB1 %u%% SB2 %u%%", state.speedTop, state.speedBottom);
  }
}

void Feedback::composeOta(char* line1, char* line2) {
  if (strcmp(otaState_, "error") == 0) {
    strcpy(line1, "Update failed");
    snprintf(line2, 24, "%s", otaError_);
    return;
  }
  if (strcmp(otaState_, "rebooting") == 0) {
    strcpy(line1, "Update done");
    strcpy(line2, "Rebooting...");
    return;
  }
  strcpy(line1, "Updating fw...");
  char bar[11];
  int filled = constrain(otaProgress_, 0, 100) / 10;
  for (int i = 0; i < 10; i++) {
    bar[i] = i < filled ? static_cast<char>(0xFF) : '-';
  }
  bar[10] = '\0';
  snprintf(line2, 24, "%s %3d%%", bar, otaProgress_);
}

const char* Feedback::wifiErrorText(const String& code) {
  if (code == "no_ssid") return "No such network";
  if (code == "auth_failed") return "Wrong password";
  if (code == "timeout") return "Timed out";
  if (code == "weak_security") return "Unsupported auth";
  return "Try again";
}
