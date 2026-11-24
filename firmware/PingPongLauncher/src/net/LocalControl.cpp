#include "LocalControl.h"

#include <ESPmDNS.h>
#include <WiFi.h>

#include "../config/AppConfig.h"
#include "WebAppAssets.h"

void LocalControl::begin(CommandHandler onCommand, JsonProvider stateJson, JsonProvider infoJson) {
  onCommand_ = onCommand;
  stateJson_ = stateJson;
  infoJson_ = infoJson;
}

void LocalControl::attach(WebServer& server) {
  for (size_t i = 0; i < WEBAPP_FILE_COUNT; i++) {
    server.on(WEBAPP_FILES[i].path, HTTP_GET, [this, &server, i]() { sendFile(server, i); });
  }
  server.on("/app", HTTP_GET, [&server]() {
    server.sendHeader("Location", "/app/");
    server.send(302, "text/plain", "");
  });
  server.on("/api/info", HTTP_GET, [this, &server]() {
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", infoJson_());
  });
  server.on("/api/state", HTTP_GET, [this, &server]() {
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", stateJson_());
  });
  server.on("/api/cmd", HTTP_POST, [this, &server]() { handleCommand(server); });
}

void LocalControl::update(bool serve) {
  if (serve && !serving_) {
    if (!attached_) {
      attach(server_);
      server_.on("/", HTTP_GET, [this]() {
        server_.sendHeader("Location", "/app/");
        server_.send(302, "text/plain", "");
      });
      attached_ = true;
    }
    server_.begin();
    serving_ = true;
    if (!mdnsStarted_ && MDNS.begin(AppConfig::LOCAL_HOSTNAME)) {
      MDNS.addService("http", "tcp", AppConfig::LOCAL_HTTP_PORT);
      mdnsStarted_ = true;
    }
    Serial.printf("[Local] Control at http://%s/app/ or http://%s.local/app/\n",
                  WiFi.localIP().toString().c_str(), AppConfig::LOCAL_HOSTNAME);
  } else if (!serve && serving_) {
    server_.stop();
    serving_ = false;
    Serial.println("[Local] Stopped");
  }
  if (serving_) {
    server_.handleClient();
  }
}

void LocalControl::sendFile(WebServer& server, size_t index) {
  const WebAppFile& file = WEBAPP_FILES[index];
  server.sendHeader("Content-Encoding", "gzip");
  server.sendHeader("Cache-Control", file.immutable ? "public, max-age=31536000, immutable" : "no-cache");
  server.send_P(200, file.mime, reinterpret_cast<const char*>(file.data), file.length);
}

void LocalControl::handleCommand(WebServer& server) {
  server.sendHeader("Cache-Control", "no-store");
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"empty\"}");
    return;
  }
  const String& body = server.arg("plain");
  String ack = onCommand_(reinterpret_cast<const uint8_t*>(body.c_str()), body.length());
  if (ack.isEmpty()) {
    server.send(503, "application/json", "{\"error\":\"busy\"}");
    return;
  }
  server.send(200, "application/json", ack);
}
