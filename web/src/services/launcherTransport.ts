import type { LauncherCommand } from "../types/launcher";
import { LauncherLocal } from "./launcherLocal";
import { LauncherMqtt, type LauncherMqttHandlers } from "./launcherMqtt";

/**
 * Message channel to one launcher: MQTT through the broker or HTTP to the device.
 */
export interface LauncherTransport {
  connect(deviceId: string, handlers: LauncherMqttHandlers): void;
  publish(command: LauncherCommand & { id?: string }): boolean;
  disconnect(): void;
}

/**
 * Picks the transport for this build.
 */
export function createTransport(): LauncherTransport {
  if (import.meta.env.VITE_TRANSPORT === "local") {
    return new LauncherLocal();
  }
  return new LauncherMqtt();
}
