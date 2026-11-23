#include "MqttLink.h"

#include "../config/AppConfig.h"

void MqttLink::begin(const String& deviceId, MessageHandler onCommand,
                     ConnectedHandler onConnected) {
  onCommand_ = onCommand;
  onConnected_ = onConnected;

  String base = String(AppConfig::MQTT_TOPIC_ROOT) + "/" + deviceId + "/";
  topicCmd_ = base + "cmd";
  topicState_ = base + "state";
  topicStatus_ = base + "status";
  topicOta_ = base + "ota";
  clientId_ = "ppl-" + deviceId + "-" + String(esp_random() & 0xFFFF, HEX);

  client_.setServer(AppConfig::MQTT_HOST, AppConfig::MQTT_PORT);
  client_.setBufferSize(AppConfig::MQTT_BUFFER_SIZE);
  client_.setKeepAlive(AppConfig::MQTT_KEEPALIVE_S);
  client_.setCallback([this](char* topic, uint8_t* payload, unsigned int length) {
    if (topicCmd_.equals(topic) && onCommand_) {
      onCommand_(payload, length);
    }
  });
}

void MqttLink::loop(uint32_t nowMs) {
  if (client_.connected()) {
    client_.loop();
    return;
  }
  if (attempted_ && nowMs - lastAttemptMs_ < AppConfig::MQTT_RETRY_MS) {
    return;
  }
  attempted_ = true;
  lastAttemptMs_ = nowMs;
  connect();
}

bool MqttLink::publishState(const char* json) {
  if (!client_.connected()) {
    return false;
  }
  return client_.publish(topicState_.c_str(), json, true);
}

bool MqttLink::publishOta(const char* json) {
  if (!client_.connected()) {
    return false;
  }
  return client_.publish(topicOta_.c_str(), json, false);
}

bool MqttLink::connect() {
  Serial.printf("[MQTT] Connecting to %s as %s\n", AppConfig::MQTT_HOST, clientId_.c_str());
  bool ok = client_.connect(clientId_.c_str(), nullptr, nullptr, topicStatus_.c_str(), 1, true,
                            "offline", true);
  if (!ok) {
    Serial.printf("[MQTT] Connect failed, rc=%d\n", client_.state());
    return false;
  }
  client_.publish(topicStatus_.c_str(), "online", true);
  client_.subscribe(topicCmd_.c_str(), 1);
  Serial.printf("[MQTT] Connected, listening on %s\n", topicCmd_.c_str());
  if (onConnected_) {
    onConnected_();
  }
  return true;
}
