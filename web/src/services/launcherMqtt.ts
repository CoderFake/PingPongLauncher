import mqtt, { type MqttClient } from "mqtt";

import { buildTopics, MQTT_URL, type DeviceTopics } from "../config/mqtt";
import type { BrokerStatus, LauncherCommand, LauncherState, OtaStatus } from "../types/launcher";
import { makeCommandId } from "../utils/commandId";
import { parseState } from "../utils/parseState";

/**
 * Callbacks fired by LauncherMqtt as messages and connection events arrive.
 */
export interface LauncherMqttHandlers {
  onState: (state: LauncherState) => void;
  onDeviceStatus: (online: boolean) => void;
  onBrokerStatus: (status: BrokerStatus) => void;
  onOta: (status: OtaStatus) => void;
}

const DUPLICATE_WINDOW_MS = 300;

/**
 * Browser-side MQTT session for one launcher device.
 * Commands carry a unique id and identical consecutive commands are suppressed.
 */
export class LauncherMqtt {
  private client: MqttClient | null = null;
  private topics: DeviceTopics | null = null;
  private lastBody = "";
  private lastSentAt = 0;

  /**
   * Opens the broker connection and subscribes to the device topics.
   */
  connect(deviceId: string, handlers: LauncherMqttHandlers): void {
    this.disconnect();
    const topics = buildTopics(deviceId);
    this.topics = topics;

    handlers.onBrokerStatus("connecting");
    const client = mqtt.connect(MQTT_URL, {
      clientId: `ppl-web-${makeCommandId()}`,
      clean: true,
      keepalive: 30,
      reconnectPeriod: 2000,
      connectTimeout: 10000,
    });
    this.client = client;

    client.on("connect", () => {
      handlers.onBrokerStatus("connected");
      client.subscribe([topics.state, topics.status, topics.ota], { qos: 1 }, (err) => {
        if (!err) {
          this.send({ get: true });
        }
      });
    });
    client.on("reconnect", () => handlers.onBrokerStatus("reconnecting"));
    client.on("offline", () => handlers.onBrokerStatus("offline"));
    client.on("error", () => handlers.onBrokerStatus("offline"));

    client.on("message", (topic, payload) => {
      const text = payload.toString();
      if (topic === topics.status) {
        handlers.onDeviceStatus(text === "online");
        return;
      }
      if (topic === topics.ota) {
        try {
          handlers.onOta(JSON.parse(text) as OtaStatus);
        } catch {
          return;
        }
        return;
      }
      if (topic === topics.state) {
        const state = parseState(text);
        if (state) {
          handlers.onState(state);
        }
      }
    });
  }

  /**
   * Publishes a command unless it repeats the previous one within a short window.
   */
  send(command: LauncherCommand): void {
    if (!this.client || !this.topics || !this.client.connected) {
      return;
    }
    const body = JSON.stringify(command);
    const now = Date.now();
    if (!command.get && body === this.lastBody && now - this.lastSentAt < DUPLICATE_WINDOW_MS) {
      return;
    }
    this.lastBody = body;
    this.lastSentAt = now;

    const payload = JSON.stringify({ id: makeCommandId(), ...command });
    this.client.publish(this.topics.cmd, payload, { qos: 1, retain: false });
  }

  /**
   * Closes the connection and drops all listeners.
   */
  disconnect(): void {
    if (this.client) {
      this.client.removeAllListeners();
      this.client.end(true);
    }
    this.client = null;
    this.topics = null;
    this.lastBody = "";
  }
}
