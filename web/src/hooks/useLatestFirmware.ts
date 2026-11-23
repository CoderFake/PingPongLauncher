import { useCallback, useEffect, useState } from "react";

import { fetchLatestFirmware, type FirmwareRelease } from "../services/firmwareReleases";

/**
 * Value returned by useLatestFirmware.
 */
export interface LatestFirmware {
  release: FirmwareRelease | null;
  error: string | null;
  loading: boolean;
  reload: () => void;
}

/**
 * Fetches the newest firmware release once and on demand.
 */
export function useLatestFirmware(): LatestFirmware {
  const [release, setRelease] = useState<FirmwareRelease | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [loading, setLoading] = useState(false);

  const reload = useCallback(() => {
    setLoading(true);
    setError(null);
    fetchLatestFirmware()
      .then(setRelease)
      .catch((e: Error) => setError(e.message))
      .finally(() => setLoading(false));
  }, []);

  useEffect(reload, [reload]);

  return { release, error, loading, reload };
}
