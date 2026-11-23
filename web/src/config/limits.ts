/**
 * Control ranges mirrored from firmware AppConfig.h.
 */
export const LIMITS = {
  speed: { min: 0, max: 100 },
  turretRange: { min: 5, max: 80 },
  turretAngle: { min: -80, max: 80 },
  feed: { min: 10, max: 80 },
} as const;

/**
 * How often edits are batched into one command while a control is being dragged.
 */
export const FLUSH_INTERVAL_MS = 120;

/**
 * How long to wait for the device to acknowledge a command before resending it.
 */
export const ACK_TIMEOUT_MS = 2500;

/**
 * Attempts per command (first send included) before the edit is dropped.
 */
export const MAX_SEND_ATTEMPTS = 3;

/**
 * How long an acknowledged edit waits for the matching state before trusting the device.
 */
export const STATE_AFTER_ACK_TIMEOUT_MS = 3000;
