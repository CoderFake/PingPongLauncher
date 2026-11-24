/**
 * True for the build embedded in the firmware: the page talks to the
 * device that served it over HTTP instead of going through the MQTT broker.
 */
export const LOCAL_MODE: boolean = import.meta.env.VITE_TRANSPORT === "local";

/**
 * Local API polling period and per-request timeout.
 */
export const LOCAL_POLL_MS = 300;
export const LOCAL_REQUEST_TIMEOUT_MS = 2500;
