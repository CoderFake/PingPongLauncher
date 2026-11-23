#pragma once

#include <Arduino.h>

#include <functional>

/**
 * ArduinoOTA on the local network: the board shows up as a network port
 * named pingpong-<deviceId> in Arduino IDE.
 */
class LocalOta {
 public:
  using StartHandler = std::function<void()>;

  /**
   * Starts the OTA listener; `onStart` runs before flashing begins.
   */
  void begin(const String& deviceId, StartHandler onStart);

  /**
   * Services OTA requests; call every loop while online.
   */
  void handle();

 private:
  bool started_ = false;
};
