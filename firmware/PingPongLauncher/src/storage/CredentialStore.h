#pragma once

#include <Arduino.h>

/**
 * Persists WiFi credentials in NVS so they survive reboots.
 */
class CredentialStore {
 public:
  /**
   * Loads saved credentials; returns false when none are stored.
   */
  bool load(String& ssid, String& password);

  /**
   * Saves credentials, replacing any previous values.
   */
  void save(const String& ssid, const String& password);

  /**
   * Erases stored credentials.
   */
  void clear();
};
