import { useCallback, useEffect, useRef, useState } from "react";

import { LauncherMqtt } from "../services/launcherMqtt";
import type { BrokerStatus, LauncherCommand, LauncherState, OtaStatus } from "../types/launcher";

/**
 * Value returned by useLauncher.
 */
export interface LauncherHandle {
  state: LauncherState | null;
  deviceOnline: boolean;
  broker: BrokerStatus;
  ota: OtaStatus | null;
  send: (command: LauncherCommand) => void;
}

/**
 * Connects to one device and keeps its state in sync.
 * Messages with an older revision from the same boot are discarded.
 */
export function useLauncher(deviceId: string | null): LauncherHandle {
  const [state, setState] = useState<LauncherState | null>(null);
  const [deviceOnline, setDeviceOnline] = useState(false);
  const [broker, setBroker] = useState<BrokerStatus>("idle");
  const [ota, setOta] = useState<OtaStatus | null>(null);
  const serviceRef = useRef<LauncherMqtt | null>(null);
  const versionRef = useRef({ boot: -1, rev: 0 });

  useEffect(() => {
    if (!deviceId) {
      return;
    }
    const service = new LauncherMqtt();
    serviceRef.current = service;
    versionRef.current = { boot: -1, rev: 0 };
    setState(null);
    setDeviceOnline(false);
    setOta(null);

    service.connect(deviceId, {
      onState: (next) => {
        const seen = versionRef.current;
        if (next.boot === seen.boot && next.rev < seen.rev) {
          return;
        }
        if (next.boot !== seen.boot && seen.boot !== -1) {
          setOta(null);
        }
        versionRef.current = { boot: next.boot, rev: next.rev };
        setState(next);
      },
      onDeviceStatus: setDeviceOnline,
      onBrokerStatus: setBroker,
      onOta: setOta,
    });

    return () => {
      service.disconnect();
      serviceRef.current = null;
      setBroker("idle");
    };
  }, [deviceId]);

  const send = useCallback((command: LauncherCommand) => {
    serviceRef.current?.send(command);
  }, []);

  return { state, deviceOnline, broker, ota, send };
}
