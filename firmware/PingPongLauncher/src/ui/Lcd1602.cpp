#include "Lcd1602.h"

#include <Wire.h>

#include "../config/AppConfig.h"

namespace {

constexpr uint8_t BIT_RS = 0x01;
constexpr uint8_t BIT_EN = 0x04;
constexpr uint8_t BIT_BACKLIGHT = 0x08;

constexpr uint8_t CMD_CLEAR = 0x01;
constexpr uint8_t CMD_ENTRY_MODE = 0x06;
constexpr uint8_t CMD_DISPLAY_ON = 0x0C;
constexpr uint8_t CMD_FUNCTION_4BIT_2LINE = 0x28;
constexpr uint8_t CMD_SET_DDRAM = 0x80;
constexpr uint8_t ROW_OFFSETS[] = {0x00, 0x40};

/**
 * Appends one 4-bit transfer (with enable pulse) to the open I2C transmission.
 */
void queueNibble(uint8_t nibble, bool data) {
  uint8_t bits = (nibble << 4) | BIT_BACKLIGHT | (data ? BIT_RS : 0);
  Wire.write(bits | BIT_EN);
  Wire.write(bits);
}

/**
 * Appends one full byte as two nibbles to the open I2C transmission.
 */
void queueByte(uint8_t value, bool data) {
  queueNibble(value >> 4, data);
  queueNibble(value & 0x0F, data);
}

}

void Lcd1602::begin(uint32_t nowMs) {
  Wire.begin(AppConfig::PIN_I2C_SDA, AppConfig::PIN_I2C_SCL, AppConfig::LCD_I2C_HZ);
  Wire.setTimeOut(20);
  lastProbeMs_ = nowMs;
  if (probe() && init()) {
    Serial.printf("[LCD] Found at 0x%02X\n", address_);
  } else {
    address_ = 0;
    Serial.println("[LCD] Not connected, running without display");
  }
}

void Lcd1602::update(uint32_t nowMs) {
  if (present()) {
    for (uint8_t row = 0; row < 2; row++) {
      if (strcmp(shown_[row], wanted_[row]) != 0) {
        if (!writeLine(row, wanted_[row])) {
          markLost();
          return;
        }
        strcpy(shown_[row], wanted_[row]);
      }
    }
    return;
  }
  if (nowMs - lastProbeMs_ < AppConfig::LCD_PROBE_INTERVAL_MS) {
    return;
  }
  lastProbeMs_ = nowMs;
  if (probe() && init()) {
    Serial.printf("[LCD] Connected at 0x%02X\n", address_);
  } else {
    address_ = 0;
  }
}

void Lcd1602::show(const char* line1, const char* line2) {
  snprintf(wanted_[0], sizeof(wanted_[0]), "%-16.16s", line1);
  snprintf(wanted_[1], sizeof(wanted_[1]), "%-16.16s", line2);
}

bool Lcd1602::probe() {
  for (uint8_t candidate : AppConfig::LCD_ADDRESSES) {
    Wire.beginTransmission(candidate);
    if (Wire.endTransmission() == 0) {
      address_ = candidate;
      return true;
    }
  }
  return false;
}

bool Lcd1602::init() {
  delay(50);
  if (!sendNibble(0x03, false)) return false;
  delay(5);
  if (!sendNibble(0x03, false)) return false;
  delayMicroseconds(150);
  if (!sendNibble(0x03, false)) return false;
  if (!sendNibble(0x02, false)) return false;
  if (!sendCommand(CMD_FUNCTION_4BIT_2LINE)) return false;
  if (!sendCommand(CMD_DISPLAY_ON)) return false;
  if (!sendCommand(CMD_CLEAR)) return false;
  delay(2);
  if (!sendCommand(CMD_ENTRY_MODE)) return false;
  shown_[0][0] = '\0';
  shown_[1][0] = '\0';
  return true;
}

bool Lcd1602::writeLine(uint8_t row, const char* text) {
  Wire.beginTransmission(address_);
  queueByte(CMD_SET_DDRAM | ROW_OFFSETS[row], false);
  for (uint8_t i = 0; i < COLS; i++) {
    queueByte(text[i] ? static_cast<uint8_t>(text[i]) : ' ', true);
  }
  return Wire.endTransmission() == 0;
}

bool Lcd1602::sendNibble(uint8_t nibble, bool data) {
  Wire.beginTransmission(address_);
  queueNibble(nibble, data);
  return Wire.endTransmission() == 0;
}

bool Lcd1602::sendCommand(uint8_t command) {
  Wire.beginTransmission(address_);
  queueByte(command, false);
  return Wire.endTransmission() == 0;
}

void Lcd1602::markLost() {
  Serial.println("[LCD] Disconnected");
  address_ = 0;
}
