#pragma once

#include <Arduino.h>

#include <functional>

/**
 * A firmware update requested over MQTT.
 */
struct OtaRequest {
  String url;
  String sha256;
  String version;
};

/**
 * Outcome of one OTA attempt; `error` is a short machine-readable code.
 */
struct OtaResult {
  bool ok;
  String error;
};

/**
 * Downloads a firmware image over HTTPS into the inactive OTA slot,
 * verifies its SHA-256 and only then marks it as the next boot image.
 */
class OtaUpdater {
 public:
  using ProgressCallback = std::function<void(uint8_t percent)>;

  /**
   * Returns an error code when the request is not allowed, or an empty string.
   */
  static String validate(const OtaRequest& request);

  /**
   * Runs the update synchronously, reporting progress in 5% steps.
   */
  OtaResult run(const OtaRequest& request, ProgressCallback onProgress);
};
