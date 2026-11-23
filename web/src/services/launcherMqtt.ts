import mqtt, { type MqttClient } from "mqtt";

import { buildTopics, MQTT_URL, type DeviceTopics } from "../config/mqtt";
import type { AckMessage, BrokerStatus, LauncherCommand, LauncherState, OtaStatus } from "../types/launcher";
import { makeCommandId } from "../utils/commandId";
import { parseAck, parseState } from "../utils/parseState";

/**
 * Callbacks fired by LauncherMqtt as messages and connection events arrive.
 */
export interface LauncherMqttHandlers {
  onState: (state: LauncherState) => void;
  onAck: (ack: AckMessage) => void;
  onDeviceStatus: (online: boolean) => void;
  onBrokerStatus: (status: BrokerStatus) => void;
  onOta: (status: OtaStatus) => void;
  onConnected: () => void;
}

/**
 * Browser-side MQTT session for one launcher device.
 * It only moves messages; retries and reconciliation live in SyncEngine.
 */
export class LauncherMqtt {
  private client: MqttClient | null = null;
  private topics: DeviceTopics | null = null;

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
      client.subscribe([topics.state, topics.status, topics.ota, topics.ack], { qos: 1 }, (err) => {
        if (!err) {
          handlers.onConnected();
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
      } else if (topic === topics.state) {
        const state = parseState(text);
        if (state) handlers.onState(state);
      } else if (topic === topics.ack) {
        const ack = parseAck(text);
        if (ack) handlers.onAck(ack);
      } else if (topic === topics.ota) {
        try {
          handlers.onOta(JSON.parse(text) as OtaStatus);
        } catch {
          return;
        }
      }
    });
  }

  /**
   * Publishes a command; a missing id is generated. Returns false while offline.
   */
  publish(command: LauncherCommand & { id?: string }): boolean {
    if (!this.client || !this.topics || !this.client.connected) {
      return false;
    }
    const payload = JSON.stringify({ ...command, id: command.id ?? makeCommandId() });
    this.client.publish(this.topics.cmd, payload, { qos: 1, retain: false });
    return true;
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
  }
}
