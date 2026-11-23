#pragma once

#include <Arduino.h>

#include <functional>

#include "../net/MqttLink.h"
#include "OtaGuard.h"
#include "OtaUpdater.h"

/**
 * Accepts OTA requests from MQTT, runs them outside the MQTT callback,
 * reports progress on the ota topic and arms the rollback guard.
 */
class OtaCoordinator {
 public:
  using StatusHook = std::function<void(const char* state, int progress, const char* error)>;

  /**
   * Registers a local observer of OTA progress (LCD, buzzer).
   */
  void onStatus(StatusHook hook) { hook_ = hook; }

  /**
   * Runs the boot-time rollback check; call first in setup().
   */
  void bootCheck();

  /**
   * Binds the MQTT link used for progress reports.
   */
  void attach(MqttLink& mqtt);

  /**
   * Queues a request; rejects it right away when the launcher is running.
   */
  void request(const OtaRequest& request, bool launcherPowered);

  /**
   * Confirms the running firmware after a successful MQTT connection.
   */
  void markHealthy() { guard_.markHealthy(); }

  /**
   * Returns true while an update is queued or running.
   */
  bool busy() const { return pending_ || running_; }

  /**
   * Executes a queued update; reboots on success. Call every loop.
   */
  void update();

 private:
  void report(const char* state, int progress, const char* error);

  OtaGuard guard_;
  OtaUpdater updater_;
  MqttLink* mqtt_ = nullptr;
  StatusHook hook_;
  OtaRequest request_;
  bool pending_ = false;
  bool running_ = false;
};
