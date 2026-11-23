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
 * Minimum interval between commands sent while a slider is dragged.
 */
export const SEND_INTERVAL_MS = 150;

/**
 * How long a released slider keeps its local value while waiting for the device echo.
 */
export const ECHO_TIMEOUT_MS = 1500;
