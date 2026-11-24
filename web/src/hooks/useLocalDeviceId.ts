import { useEffect, useState } from "react";

import { LOCAL_MODE } from "../config/transport";

/**
 * In the embedded build, asks the serving device for its id; retries until it answers.
 * Always null in the cloud build.
 */
export function useLocalDeviceId(): string | null {
  const [deviceId, setDeviceId] = useState<string | null>(null);

  useEffect(() => {
    if (!LOCAL_MODE) {
      return;
    }
    let cancelled = false;
    let timer = 0;
    const load = async () => {
      try {
        const response = await fetch("/api/info", { cache: "no-store" });
        const info = (await response.json()) as { id?: unknown };
        if (!cancelled && typeof info.id === "string") {
          setDeviceId(info.id);
          return;
        }
      } catch {
        if (cancelled) return;
      }
      timer = window.setTimeout(load, 1500);
    };
    void load();
    return () => {
      cancelled = true;
      window.clearTimeout(timer);
    };
  }, []);

  return deviceId;
}
