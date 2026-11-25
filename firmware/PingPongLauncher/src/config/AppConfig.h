#pragma once

#include <Arduino.h>

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "1.2.3"
#endif

/**
 * Compile-time configuration: pins, PWM channels, motion limits,
 * network settings and default launcher values.
 */
namespace AppConfig {

constexpr uint8_t PIN_FLYWHEEL_TOP = 18;
constexpr uint8_t PIN_FLYWHEEL_BOTTOM = 19;
constexpr uint8_t PIN_SERVO_FEED = 25;
constexpr uint8_t PIN_SERVO_TURRET = 26;
constexpr uint8_t PIN_RESET_BUTTON = 0;
constexpr uint8_t PIN_STATUS_LED = 2;
constexpr uint8_t PIN_BUZZER = 27;
constexpr uint8_t PIN_I2C_SDA = 21;
constexpr uint8_t PIN_I2C_SCL = 22;
constexpr uint8_t PIN_BTN_POWER = 32;
constexpr uint8_t PIN_BTN_MENU = 33;
constexpr uint8_t PIN_BTN_MINUS = 13;
constexpr uint8_t PIN_BTN_PLUS = 14;

constexpr uint8_t CH_FLYWHEEL_TOP = 0;
constexpr uint8_t CH_FLYWHEEL_BOTTOM = 1;
constexpr uint8_t CH_SERVO_FEED = 2;
constexpr uint8_t CH_SERVO_TURRET = 3;
constexpr uint8_t CH_BUZZER = 8;

constexpr uint32_t FLYWHEEL_PWM_FREQ = 1000;
constexpr uint8_t FLYWHEEL_PWM_BITS = 10;
constexpr uint8_t FLYWHEEL_RAMP_STEP = 2;
constexpr uint32_t FLYWHEEL_RAMP_INTERVAL_MS = 20;
constexpr uint32_t FLYWHEEL_SETTLE_MS = 1500;

constexpr uint32_t SERVO_PWM_FREQ = 50;
constexpr uint8_t SERVO_PWM_BITS = 16;
constexpr uint16_t SERVO_MIN_US = 500;
constexpr uint16_t SERVO_MAX_US = 2500;

constexpr float TURRET_CENTER_DEG = 90.0f;
constexpr int8_t TURRET_LIMIT_DEG = 80;
constexpr bool TURRET_INVERT = false;
constexpr float TURRET_SWEEP_DEG_PER_SEC = 40.0f;
constexpr float TURRET_MAX_DEG_PER_SEC = 120.0f;

constexpr float FEED_DOWN_DEG = 0.0f;
constexpr float FEED_UP_DEG = 170.0f;
constexpr uint32_t SERVO_MS_PER_60DEG = 200;
constexpr uint32_t FEED_HOLD_UP_MS = 120;
constexpr uint32_t FEED_SETTLE_DOWN_MS = 250;
constexpr uint8_t FEED_RATE_MIN = 10;
constexpr uint8_t FEED_RATE_MAX = 35;

constexpr bool BUZZER_ACTIVE_LOW = true;
constexpr uint8_t BUZZER_PWM_BITS = 8;

constexpr uint8_t LCD_ADDRESSES[] = {0x27, 0x3F};
constexpr uint32_t LCD_I2C_HZ = 100000;
constexpr uint32_t LCD_PROBE_INTERVAL_MS = 3000;
constexpr uint32_t LCD_PAGE_INTERVAL_MS = 3000;
constexpr uint32_t FEEDBACK_INTERVAL_MS = 100;

constexpr uint8_t DEFAULT_SPEED_TOP = 80;
constexpr uint8_t DEFAULT_SPEED_BOTTOM = 60;
constexpr uint8_t DEFAULT_TURRET_RANGE = 25;
constexpr uint8_t TURRET_RANGE_MIN = 5;
constexpr uint8_t DEFAULT_FEED_RATE = 30;

constexpr const char* AP_SSID_PREFIX = "PingPong-";
constexpr const char* AP_PASSWORD = "pingpong";
constexpr uint32_t STA_CONNECT_TIMEOUT_MS = 22000;
constexpr uint32_t PORTAL_SWITCH_DELAY_MS = 20000;
constexpr uint32_t RESET_HOLD_MS = 3000;

constexpr uint32_t BUTTON_DEBOUNCE_MS = 25;
constexpr uint32_t BUTTON_REPEAT_DELAY_MS = 450;
constexpr uint32_t BUTTON_REPEAT_MS = 90;
constexpr uint32_t BUTTON_LONG_MS = 1000;
constexpr uint32_t PANEL_IDLE_MS = 6000;
constexpr uint32_t PANEL_FAST_AFTER_MS = 1500;
constexpr uint8_t PANEL_FAST_STEP = 5;

constexpr const char* LOCAL_HOSTNAME = "pingpong";
constexpr uint16_t LOCAL_HTTP_PORT = 80;

constexpr const char* MQTT_HOST = "broker.emqx.io";
constexpr uint16_t MQTT_PORT = 1883;
constexpr const char* MQTT_TOPIC_ROOT = "ppl";
constexpr uint16_t MQTT_KEEPALIVE_S = 30;
constexpr uint32_t MQTT_RETRY_MS = 3000;
constexpr uint16_t MQTT_BUFFER_SIZE = 512;
constexpr uint32_t STATE_PUBLISH_MIN_GAP_MS = 80;

constexpr const char* OTA_URL_PREFIX =
    "https://github.com/CoderFake/PingPongLauncher/releases/download/";
constexpr const char* OTA_LOCAL_PASSWORD = "pingpong-ota";
constexpr uint32_t OTA_HTTP_TIMEOUT_MS = 15000;
constexpr uint8_t OTA_MAX_TRIAL_BOOTS = 3;

}
