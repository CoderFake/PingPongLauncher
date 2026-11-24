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

  /**
   * Returns true while the setup portal owns port 80 on the soft AP.
   */
  bool portalRunning() const { return portal_.running(); }

  /**
   * Registers extra routes on the setup portal's HTTP server.
   */
  void addPortalRoutes(const std::function<void(WebServer&)>& installer) { portal_.addRoutes(installer); }

 private:
  enum class Phase : uint8_t { StaConnecting, Portal, PortalConnecting, PortalSuccess, Online };

  void startPortal();
  void finishPortal();
  void requestConnect(const String& ssid, const String& password);
  void checkPortalAttempt(uint32_t nowMs);
  void startAttempt(bool pinToScannedAp);
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
  uint32_t attemptStartMs_ = 0;
  bool attemptStarted_ = false;
  bool retried_ = false;
  bool targetKnown_ = false;
  int32_t targetChannel_ = 0;
  uint8_t targetBssid_[6] = {};
  volatile uint8_t lastReason_ = 0;
  volatile uint8_t authFailures_ = 0;
};
