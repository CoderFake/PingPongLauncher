#pragma once

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>

#include <functional>

/**
 * MQTT session to the public broker with device-scoped topics:
 *   <root>/<deviceId>/cmd     client commands (subscribed)
 *   <root>/<deviceId>/state   full state, retained
 *   <root>/<deviceId>/status  online/offline via last will, retained
 *   <root>/<deviceId>/ota     firmware update progress, not retained
 */
class MqttLink {
 public:
  using MessageHandler = std::function<void(const uint8_t* payload, size_t length)>;
  using ConnectedHandler = std::function<void()>;

  /**
   * Prepares topics and callbacks; does not connect yet.
   */
  void begin(const String& deviceId, MessageHandler onCommand, ConnectedHandler onConnected);

  /**
   * Keeps the session alive and reconnects with back-off; call every loop.
   */
  void loop(uint32_t nowMs);

  /**
   * Publishes the retained state document.
   */
  bool publishState(const char* json);

  /**
   * Publishes an OTA progress document.
   */
  bool publishOta(const char* json);

  /**
   * Services the socket without reconnecting; used during long blocking work.
   */
  void pump() { client_.loop(); }

  /**
   * Returns true while connected to the broker.
   */
  bool connected() { return client_.connected(); }

 private:
  bool connect();

  WiFiClient net_;
  PubSubClient client_{net_};
  String clientId_;
  String topicCmd_;
  String topicState_;
  String topicStatus_;
  String topicOta_;
  MessageHandler onCommand_;
  ConnectedHandler onConnected_;
  uint32_t lastAttemptMs_ = 0;
  bool attempted_ = false;
};
