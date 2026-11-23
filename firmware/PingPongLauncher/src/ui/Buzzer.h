#pragma once

#include <Arduino.h>

/**
 * Status sounds, each mapped to a short melody.
 */
enum class Sound : uint8_t {
  Boot,
  SetupNeeded,
  WifiJoined,
  WifiFailed,
  CloudOnline,
  CloudLost,
  PowerOn,
  PowerOff,
  Ready,
  OtaStart,
  OtaDone,
  Error,
  Tick,
  ResetConfirm
};

/**
 * One tone of a melody; hz = 0 is a rest.
 */
struct BuzzerNote {
  uint16_t hz;
  uint16_t ms;
};

/**
 * Non-blocking melody player for the MH-FMD passive buzzer module
 * (low-level trigger, loudest at 2-5 kHz). Tones are generated with PWM.
 */
class Buzzer {
 public:
  /**
   * Attaches the PWM channel and silences the buzzer.
   */
  void begin();

  /**
   * Starts a melody, replacing whatever is playing.
   */
  void play(Sound sound);

  /**
   * Plays a melody to the end before returning; for use right before a reboot.
   */
  void playBlocking(Sound sound);

  /**
   * Advances the current melody; call every loop.
   */
  void update(uint32_t nowMs);

 private:
  static const BuzzerNote* melody(Sound sound, uint8_t& length);
  void startNote(uint32_t nowMs);
  void tone(uint16_t hz);

  const BuzzerNote* notes_ = nullptr;
  uint8_t length_ = 0;
  uint8_t index_ = 0;
  uint32_t noteStartMs_ = 0;
};
