/**
 * Ping-pong ball launcher firmware for ESP32.
 * WiFi setup via captive portal, control and two-way sync via MQTT.
 */

#include <WiFi.h>

#include "src/config/AppConfig.h"
#include "src/core/LauncherController.h"
#include "src/core/LauncherState.h"
#include "src/core/StateCodec.h"
#include "src/input/ResetButton.h"
#include "src/net/MqttLink.h"
#include "src/net/WifiController.h"
#include "src/ota/LocalOta.h"
#include "src/ota/OtaCoordinator.h"
#include "src/ui/Feedback.h"

LauncherState state;
StateCodec codec;
LauncherController controller;
WifiController wifi;
MqttLink mqtt;
ResetButton resetButton;
OtaCoordinator ota;
LocalOta localOta;
Feedback feedback;

String deviceId;
bool publishPending = false;
uint32_t lastPublishMs = 0;
uint32_t lastFeedbackMs = 0;

/**
 * Builds a short, stable device id from the last three MAC bytes.
 */
String makeDeviceId() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char id[7];
  snprintf(id, sizeof(id), "%02X%02X%02X", mac[3], mac[4], mac[5]);
  return String(id);
}

/**
 * Handles one command payload received on the cmd topic.
 */
void onCommand(const uint8_t* payload, size_t length) {
  if (ota.busy()) {
    return;
  }
  OtaRequest otaRequest;
  CommandResult result = codec.applyCommand(payload, length, state, otaRequest);
  switch (result) {
    case CommandResult::OtaRequested:
      Serial.printf("[Core] OTA requested: %s\n", otaRequest.version.c_str());
      ota.request(otaRequest, state.power);
      break;
    case CommandResult::Changed:
      controller.apply(state);
      publishPending = true;
      Serial.printf("[Core] State rev %lu applied\n", static_cast<unsigned long>(state.rev));
      break;
    case CommandResult::StateRequested:
      publishPending = true;
      break;
    case CommandResult::Duplicate:
      Serial.println("[Core] Duplicate command ignored");
      break;
    case CommandResult::Invalid:
      Serial.println("[Core] Invalid command ignored");
      break;
    case CommandResult::NoChange:
      break;
  }
}

/**
 * Publishes state when something changed, rate-limited to coalesce bursts.
 */
void flushState(uint32_t nowMs) {
  if (!publishPending || nowMs - lastPublishMs < AppConfig::STATE_PUBLISH_MIN_GAP_MS) {
    return;
  }
  char json[AppConfig::MQTT_BUFFER_SIZE];
  codec.serialize(state, controller.telemetry(), json, sizeof(json));
  if (mqtt.publishState(json)) {
    publishPending = false;
    lastPublishMs = nowMs;
  }
}

/**
 * Forces the launcher off before a local firmware upload takes over the loop.
 */
void stopForFlashing() {
  if (state.power) {
    state.power = false;
    state.rev++;
  }
  controller.apply(state);
  controller.stopNow();
}

/**
 * Samples system state for the LCD and buzzer at a fixed rate.
 */
void updateFeedback(uint32_t nowMs) {
  feedback.tick(nowMs);
  if (nowMs - lastFeedbackMs < AppConfig::FEEDBACK_INTERVAL_MS) {
    return;
  }
  lastFeedbackMs = nowMs;
  SystemSnapshot snapshot;
  snapshot.wifi = wifi.status();
  snapshot.mqtt = mqtt.connected();
  snapshot.state = state;
  snapshot.telemetry = controller.telemetry();
  snapshot.resetHeldMs = resetButton.heldMs(nowMs);
  feedback.update(snapshot, nowMs);
}

/**
 * Drives the status LED: solid when MQTT is up, blinking otherwise.
 */
void updateStatusLed(uint32_t nowMs) {
  bool on = mqtt.connected() ? true : (nowMs / 400) % 2 == 0;
  digitalWrite(AppConfig::PIN_STATUS_LED, on ? HIGH : LOW);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  ota.bootCheck();
  pinMode(AppConfig::PIN_STATUS_LED, OUTPUT);

  WiFi.mode(WIFI_STA);
  deviceId = makeDeviceId();
  Serial.printf("\n[Boot] Ping-pong launcher %s, device id %s\n", FIRMWARE_VERSION,
                deviceId.c_str());

  controller.begin();
  controller.apply(state);
  feedback.begin(deviceId, millis());

  resetButton.begin(AppConfig::PIN_RESET_BUTTON, AppConfig::RESET_HOLD_MS,
                    []() {
                      feedback.onResetConfirmed();
                      wifi.resetCredentials();
                    });
  mqtt.begin(deviceId, onCommand, []() {
    publishPending = true;
    ota.markHealthy();
  });
  ota.attach(mqtt);
  ota.onStatus([](const char* otaState, int progress, const char* error) {
    feedback.onOta(otaState, progress, error);
  });
  wifi.begin(deviceId);
}

void loop() {
  uint32_t now = millis();

  resetButton.update(now);
  wifi.update(now);
  if (wifi.isOnline()) {
    mqtt.loop(now);
    localOta.begin(deviceId, stopForFlashing);
    localOta.handle();
  }
  ota.update();
  if (controller.update(now)) {
    publishPending = true;
  }
  flushState(now);
  updateFeedback(now);
  updateStatusLed(now);
}
