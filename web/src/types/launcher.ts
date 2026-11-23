/**
 * Turret motion modes, matching the firmware wire names.
 */
export type TurretMode = "sweep" | "random" | "fixed";

/**
 * Full state published by the device on the retained state topic.
 */
export interface LauncherState {
  rev: number;
  boot: number;
  power: boolean;
  sb1: number;
  sb2: number;
  mode: TurretMode;
  range: number;
  angle: number;
  feed: number;
  ready: boolean;
  shots: number;
  fw?: string;
}

/**
 * State fields the user can change and that are synced optimistically.
 */
export type SyncKey = "power" | "sb1" | "sb2" | "mode" | "range" | "angle" | "feed";

/**
 * Editable subset of the state.
 */
export type SyncPatch = Partial<Pick<LauncherState, SyncKey>>;

/**
 * Per-command acknowledgement published by the device on the ack topic.
 */
export interface AckMessage {
  id: string;
  rev: number;
  boot: number;
  result: "changed" | "unchanged" | "duplicate" | "state" | "ota" | "invalid";
}

/**
 * Partial update sent to the device; only included keys are applied.
 */
export interface LauncherCommand {
  power?: boolean;
  sb1?: number;
  sb2?: number;
  sbAll?: number;
  mode?: TurretMode;
  range?: number;
  angle?: number;
  feed?: number;
  get?: boolean;
  ota?: OtaCommand;
}

/**
 * Firmware image the device should download and install.
 */
export interface OtaCommand {
  url: string;
  sha256: string;
  version: string;
}

/**
 * Progress report published by the device on the ota topic.
 */
export interface OtaStatus {
  state: "queued" | "downloading" | "rebooting" | "error";
  progress: number;
  version: string;
  current: string;
  error?: string;
}

/**
 * Connection state of the browser to the MQTT broker.
 */
export type BrokerStatus = "idle" | "connecting" | "connected" | "reconnecting" | "offline";
