#pragma once

#include <Arduino.h>
#include <WebServer.h>

#include <functional>

#include "../config/AppConfig.h"

/**
 * Direct control without the cloud: serves the embedded web app under /app/
 * and a small JSON API on the local network (and on the setup AP).
 *   GET  /api/info   device id and firmware version
 *   GET  /api/state  same JSON as the MQTT state topic
 *   POST /api/cmd    same JSON as the MQTT cmd topic, answers with the ack
 */
class LocalControl {
 public:
  using CommandHandler = std::function<String(const uint8_t* payload, size_t length)>;
  using JsonProvider = std::function<String()>;

  /**
   * Stores the callbacks used by the API routes.
   */
  void begin(CommandHandler onCommand, JsonProvider stateJson, JsonProvider infoJson);

  /**
   * Registers the app and API routes on `server`.
   */
  void attach(WebServer& server);

  /**
   * Runs the station-side server while `serve` is true; call every loop.
   */
  void update(bool serve);

 private:
  void sendFile(WebServer& server, size_t index);
  void handleCommand(WebServer& server);

  WebServer server_{AppConfig::LOCAL_HTTP_PORT};
  CommandHandler onCommand_;
  JsonProvider stateJson_;
  JsonProvider infoJson_;
  bool attached_ = false;
  bool serving_ = false;
  bool mdnsStarted_ = false;
};
