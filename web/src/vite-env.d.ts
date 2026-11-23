/// <reference types="vite/client" />

/**
 * Environment variables read at build time.
 */
interface ImportMetaEnv {
  readonly VITE_MQTT_URL?: string;
  readonly VITE_MQTT_TOPIC_ROOT?: string;
  readonly VITE_GITHUB_REPO?: string;
}

interface ImportMeta {
  readonly env: ImportMetaEnv;
}
