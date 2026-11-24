#pragma once

#include <Arduino.h>
#include <DNSServer.h>
#include <WebServer.h>

#include <functional>
#include <vector>

/**
 * Captive portal served from the soft AP: lists nearby networks,
 * accepts credentials and reports connection progress as JSON.
 */
class ConfigPortal {
 public:
  using ConnectHandler = std::function<void(const String& ssid, const String& password)>;
  using StatusProvider = std::function<String()>;

  /**
   * Starts DNS hijacking and the HTTP server on the soft AP address.
   */
  void start(ConnectHandler onConnect, StatusProvider statusProvider);

  /**
   * Stops the HTTP and DNS servers.
   */
  void stop();

  /**
   * Serves pending requests; call every loop while running.
   */
  void loop();

  /**
   * Finds channel and BSSID of the strongest scanned AP with this SSID.
   */
  bool lookup(const String& ssid, int32_t& channel, uint8_t* bssid) const;

  /**
   * Returns true while the portal is serving.
   */
  bool running() const { return running_; }

  /**
   * Lets another module register extra routes on the portal's HTTP server.
   */
  void addRoutes(const std::function<void(WebServer&)>& installer) { installer(server_); }

 private:
  void handleRoot();
  void sendAsset(const char* contentType, const char* body);
  void handleScan();
  void handleConnect();
  void handleStatus();
  void handleNotFound();

  struct ScanEntry {
    String ssid;
    int32_t rssi;
    int32_t channel;
    uint8_t bssid[6];
  };

  std::vector<ScanEntry> lastScan_;
  WebServer server_{80};
  DNSServer dns_;
  bool running_ = false;
  bool routesReady_ = false;
  ConnectHandler onConnect_;
  StatusProvider statusProvider_;
};
