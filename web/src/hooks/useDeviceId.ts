import { useCallback, useState } from "react";

const STORAGE_KEY = "ppl.deviceId";

/**
 * Normalizes user input to the 6-character uppercase hex device id.
 */
export function normalizeDeviceId(input: string): string | null {
  const id = input.trim().toUpperCase();
  return /^[0-9A-F]{6}$/.test(id) ? id : null;
}

function readStored(): string | null {
  try {
    return normalizeDeviceId(localStorage.getItem(STORAGE_KEY) ?? "");
  } catch {
    return null;
  }
}

/**
 * Persists the selected device id in localStorage.
 */
export function useDeviceId(): [string | null, (id: string | null) => void] {
  const [deviceId, setDeviceId] = useState<string | null>(readStored);

  const update = useCallback((id: string | null) => {
    try {
      if (id) {
        localStorage.setItem(STORAGE_KEY, id);
      } else {
        localStorage.removeItem(STORAGE_KEY);
      }
    } catch {
      return setDeviceId(id);
    }
    setDeviceId(id);
  }, []);

  return [deviceId, update];
}
