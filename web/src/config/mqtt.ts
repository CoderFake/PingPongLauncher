/**
 * Public broker endpoint (WebSocket over TLS) and topic layout shared with the firmware.
 */
export const MQTT_URL: string = import.meta.env.VITE_MQTT_URL ?? "wss://broker.emqx.io:8084/mqtt";

export const MQTT_TOPIC_ROOT: string = import.meta.env.VITE_MQTT_TOPIC_ROOT ?? "ppl";

/**
 * Topic names scoped to one device id.
 */
export interface DeviceTopics {
  cmd: string;
  state: string;
  status: string;
  ota: string;
}

/**
 * Builds the topic set for a device id such as "A1B2C3".
 */
export function buildTopics(deviceId: string): DeviceTopics {
  const base = `${MQTT_TOPIC_ROOT}/${deviceId}`;
  return { cmd: `${base}/cmd`, state: `${base}/state`, status: `${base}/status`, ota: `${base}/ota` };
}
