import type { AckMessage, LauncherState, TurretMode } from "../types/launcher";

const MODES: readonly TurretMode[] = ["sweep", "random", "fixed"];

/**
 * Parses and validates a state payload; returns null for malformed data.
 */
export function parseState(raw: string): LauncherState | null {
  let data: unknown;
  try {
    data = JSON.parse(raw);
  } catch {
    return null;
  }
  if (typeof data !== "object" || data === null) {
    return null;
  }
  const d = data as Record<string, unknown>;
  const numbers = ["rev", "boot", "sb1", "sb2", "range", "angle", "feed", "shots"];
  if (!numbers.every((key) => typeof d[key] === "number")) {
    return null;
  }
  if (typeof d.power !== "boolean" || typeof d.ready !== "boolean") {
    return null;
  }
  if (!MODES.includes(d.mode as TurretMode)) {
    return null;
  }
  return d as unknown as LauncherState;
}

/**
 * Parses a command acknowledgement; returns null for malformed data.
 */
export function parseAck(raw: string): AckMessage | null {
  try {
    const data = JSON.parse(raw) as Record<string, unknown>;
    if (typeof data.id !== "string" || typeof data.rev !== "number" || typeof data.boot !== "number") {
      return null;
    }
    return data as unknown as AckMessage;
  } catch {
    return null;
  }
}
