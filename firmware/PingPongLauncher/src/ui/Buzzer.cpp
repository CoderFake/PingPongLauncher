#include "Buzzer.h"

#include "../config/AppConfig.h"

namespace {

using Note = BuzzerNote;

constexpr uint16_t REST = 0;

constexpr Note BOOT[] = {{1319, 70}, {REST, 30}, {1760, 90}};
constexpr Note SETUP_NEEDED[] = {{1047, 120}, {REST, 120}, {1047, 120}};
constexpr Note WIFI_JOINED[] = {{1047, 80}, {1319, 80}, {1568, 140}};
constexpr Note WIFI_FAILED[] = {{392, 400}};
constexpr Note CLOUD_ONLINE[] = {{2093, 60}};
constexpr Note CLOUD_LOST[] = {{523, 120}, {REST, 80}, {392, 160}};
constexpr Note POWER_ON[] = {{1568, 90}};
constexpr Note POWER_OFF[] = {{1568, 70}, {1047, 110}};
constexpr Note READY[] = {{2093, 50}, {REST, 50}, {2093, 50}};
constexpr Note OTA_START[] = {{1760, 60}, {REST, 40}, {1760, 60}};
constexpr Note OTA_DONE[] = {{1047, 80}, {1319, 80}, {1568, 80}, {2093, 160}};
constexpr Note ERROR_TONE[] = {{330, 250}, {REST, 80}, {330, 250}};
constexpr Note TICK[] = {{2637, 25}};
constexpr Note RESET_CONFIRM[] = {{2093, 600}};

}

/**
 * Returns a melody array and stores its length.
 */
template <size_t N>
const BuzzerNote* pick(const BuzzerNote (&notes)[N], uint8_t& length) {
  length = N;
  return notes;
}

const BuzzerNote* Buzzer::melody(Sound sound, uint8_t& length) {
  switch (sound) {
    case Sound::Boot: return pick(BOOT, length);
    case Sound::SetupNeeded: return pick(SETUP_NEEDED, length);
    case Sound::WifiJoined: return pick(WIFI_JOINED, length);
    case Sound::WifiFailed: return pick(WIFI_FAILED, length);
    case Sound::CloudOnline: return pick(CLOUD_ONLINE, length);
    case Sound::CloudLost: return pick(CLOUD_LOST, length);
    case Sound::PowerOn: return pick(POWER_ON, length);
    case Sound::PowerOff: return pick(POWER_OFF, length);
    case Sound::Ready: return pick(READY, length);
    case Sound::OtaStart: return pick(OTA_START, length);
    case Sound::OtaDone: return pick(OTA_DONE, length);
    case Sound::Error: return pick(ERROR_TONE, length);
    case Sound::Tick: return pick(TICK, length);
    case Sound::ResetConfirm: return pick(RESET_CONFIRM, length);
  }
  length = 0;
  return nullptr;
}

void Buzzer::begin() {
  ledcAttachChannel(AppConfig::PIN_BUZZER, 2000, AppConfig::BUZZER_PWM_BITS, AppConfig::CH_BUZZER);
  tone(0);
}

void Buzzer::play(Sound sound) {
  notes_ = melody(sound, length_);
  index_ = 0;
  startNote(millis());
}

void Buzzer::playBlocking(Sound sound) {
  play(sound);
  while (notes_ != nullptr) {
    update(millis());
    delay(5);
  }
}

void Buzzer::update(uint32_t nowMs) {
  if (notes_ == nullptr || nowMs - noteStartMs_ < notes_[index_].ms) {
    return;
  }
  index_++;
  if (index_ >= length_) {
    notes_ = nullptr;
    tone(0);
    return;
  }
  startNote(nowMs);
}

void Buzzer::startNote(uint32_t nowMs) {
  noteStartMs_ = nowMs;
  if (notes_ == nullptr || length_ == 0) {
    tone(0);
    return;
  }
  tone(notes_[index_].hz);
}

void Buzzer::tone(uint16_t hz) {
  const uint32_t maxDuty = (1UL << AppConfig::BUZZER_PWM_BITS) - 1;
  if (hz == 0) {
    ledcWrite(AppConfig::PIN_BUZZER, AppConfig::BUZZER_ACTIVE_LOW ? maxDuty : 0);
    return;
  }
  ledcChangeFrequency(AppConfig::PIN_BUZZER, hz, AppConfig::BUZZER_PWM_BITS);
  ledcWrite(AppConfig::PIN_BUZZER, (maxDuty + 1) / 2);
}
