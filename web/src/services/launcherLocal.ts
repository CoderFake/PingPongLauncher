import { LOCAL_POLL_MS, LOCAL_REQUEST_TIMEOUT_MS } from "../config/transport";
import type { LauncherCommand } from "../types/launcher";
import { makeCommandId } from "../utils/commandId";
import { parseAck, parseState } from "../utils/parseState";
import type { LauncherMqttHandlers } from "./launcherMqtt";

/**
 * Direct HTTP session with the device that served the page.
 * Polls /api/state and posts commands to /api/cmd; the response of each
 * command is its ack, so SyncEngine works exactly as over MQTT.
 */
export class LauncherLocal {
  private handlers: LauncherMqttHandlers | null = null;
  private timer: number | null = null;
  private active = false;
  private online = false;
  private inFlight = false;

  /**
   * Starts polling; the device id is implied by the origin.
   */
  connect(_deviceId: string, handlers: LauncherMqttHandlers): void {
    this.disconnect();
    this.handlers = handlers;
    this.active = true;
    handlers.onBrokerStatus("connecting");
    void this.poll();
  }

  /**
   * Posts a command; a missing id is generated. Returns false while unreachable.
   */
  publish(command: LauncherCommand & { id?: string }): boolean {
    if (!this.active || !this.online) {
      return false;
    }
    const body = JSON.stringify({ ...command, id: command.id ?? makeCommandId() });
    void request("/api/cmd", { method: "POST", body, headers: { "Content-Type": "application/json" } }).then(
      (text) => {
        const ack = text ? parseAck(text) : null;
        if (ack && this.active) {
          this.handlers?.onAck(ack);
          this.schedule(0);
        }
      },
    );
    return true;
  }

  /**
   * Stops polling and drops the handlers.
   */
  disconnect(): void {
    this.active = false;
    this.online = false;
    if (this.timer !== null) {
      window.clearTimeout(this.timer);
      this.timer = null;
    }
    this.handlers = null;
  }

  private schedule(delayMs: number): void {
    if (!this.active) {
      return;
    }
    if (this.timer !== null) {
      window.clearTimeout(this.timer);
    }
    this.timer = window.setTimeout(() => {
      this.timer = null;
      void this.poll();
    }, delayMs);
  }

  private async poll(): Promise<void> {
    if (!this.active || this.inFlight) {
      return;
    }
    this.inFlight = true;
    const text = await request("/api/state");
    this.inFlight = false;
    const handlers = this.handlers;
    if (!this.active || !handlers) {
      return;
    }

    const state = text ? parseState(text) : null;
    if (state) {
      if (!this.online) {
        this.online = true;
        handlers.onBrokerStatus("connected");
        handlers.onDeviceStatus(true);
        handlers.onConnected();
      }
      handlers.onState(state);
    } else if (this.online) {
      this.online = false;
      handlers.onBrokerStatus("reconnecting");
      handlers.onDeviceStatus(false);
    }
    this.schedule(LOCAL_POLL_MS);
  }
}

/**
 * Fetches a same-origin path with a timeout; returns the body or null on any failure.
 */
async function request(path: string, init?: RequestInit): Promise<string | null> {
  const controller = new AbortController();
  const timeout = window.setTimeout(() => controller.abort(), LOCAL_REQUEST_TIMEOUT_MS);
  try {
    const response = await fetch(path, { ...init, cache: "no-store", signal: controller.signal });
    return response.ok ? await response.text() : null;
  } catch {
    return null;
  } finally {
    window.clearTimeout(timeout);
  }
}
