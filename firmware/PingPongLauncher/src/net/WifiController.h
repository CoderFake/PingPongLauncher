#pragma once

#include <Arduino.h>

#include "../storage/CredentialStore.h"
#include "ConfigPortal.h"

/**
 * WiFi state as seen by the user-facing feedback (LCD, buzzer).
 */
enum class WifiState : uint8_t { Connecting, Portal, Joining, JoinFailed, Joined, Online, Lost };

/**
 * Snapshot of the WiFi link for display purposes.
 */
struct WifiStatus {
  WifiState state;
  String ssid;
  String ip;
  String apSsid;
  String error;
};

/**
 * Owns the WiFi lifecycle: boots into STA with saved credentials,
 * falls back to an AP+STA captive portal, then switches to STA only
 * once the chosen network is joined via DHCP.
 */
class WifiController {
 public:
  /**
   * Starts STA with stored credentials or opens the portal.
   */
  void begin(const String& deviceId);

  /**
   * Advances the connection state machine; call every loop.
   */
  void update(uint32_t nowMs);

  /**
   * Returns true when the station link has an IP address.
   */
  bool isOnline() const;

  /**
   * Returns the current link state for display.
   */
  WifiStatus status() const;

  /**
   * Erases saved credentials and reboots into the portal.
   */
  void resetCredentials();

 private:
  enum class Phase : uint8_t { StaConnecting, Portal, PortalConnecting, PortalSuccess, Online };

  void startPortal();
  void finishPortal();
  void requestConnect(const String& ssid, const String& password);
  void checkPortalAttempt(uint32_t nowMs);
  String statusJson() const;
  void setPhase(Phase phase);

  CredentialStore store_;
  ConfigPortal portal_;
  Phase phase_ = Phase::StaConnecting;
  String deviceId_;
  String apSsid_;
  String pendingSsid_;
  String pendingPassword_;
  String lastError_;
  bool hasSaved_ = false;
  uint32_t phaseStartMs_ = 0;
};
