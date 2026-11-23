import { useCallback, useEffect, useRef, useState } from "react";

import { FLUSH_INTERVAL_MS } from "../config/limits";
import { LauncherMqtt } from "../services/launcherMqtt";
import { SyncEngine } from "../sync/syncEngine";
import type {
  BrokerStatus,
  LauncherCommand,
  LauncherState,
  OtaStatus,
  SyncKey,
  SyncPatch,
} from "../types/launcher";

/**
 * Value returned by useLauncher.
 */
export interface LauncherHandle {
  state: LauncherState | null;
  pending: Set<SyncKey>;
  failed: SyncKey[];
  deviceOnline: boolean;
  broker: BrokerStatus;
  ota: OtaStatus | null;
  set: (patch: SyncPatch) => void;
  sendOnce: (command: LauncherCommand) => void;
}

/**
 * Connects to one device and keeps the UI in sync with it through SyncEngine.
 * Edits show instantly, are batched every FLUSH_INTERVAL_MS, and stay on screen
 * until the device confirms them.
 */
export function useLauncher(deviceId: string | null): LauncherHandle {
  const [engine0] = useState(() => new SyncEngine());
  const engineRef = useRef(engine0);
  const serviceRef = useRef<LauncherMqtt | null>(null);
  const [state, setState] = useState<LauncherState | null>(null);
  const [pending, setPending] = useState<Set<SyncKey>>(new Set());
  const [failed, setFailed] = useState<SyncKey[]>([]);
  const [deviceOnline, setDeviceOnline] = useState(false);
  const [broker, setBroker] = useState<BrokerStatus>("idle");
  const [ota, setOta] = useState<OtaStatus | null>(null);

  const render = useCallback(() => {
    const engine = engineRef.current;
    setState(engine.view());
    setPending(engine.pendingKeys());
  }, []);

  useEffect(() => {
    if (!deviceId) {
      return;
    }
    const engine = engineRef.current;
    const service = new LauncherMqtt();
    serviceRef.current = service;
    engine.reset();
    render();
    setDeviceOnline(false);
    setOta(null);
    setFailed([]);

    service.connect(deviceId, {
      onState: (next) => {
        if (engine.onState(next)) render();
      },
      onAck: (ack) => {
        engine.onAck(ack, Date.now());
        render();
      },
      onDeviceStatus: setDeviceOnline,
      onBrokerStatus: setBroker,
      onOta: setOta,
      onConnected: () => service.publish({ get: true }),
    });

    const timer = window.setInterval(() => {
      const now = Date.now();
      const command = engine.flush(now);
      if (command) service.publish(command);
      const { commands, failed: lost } = engine.tick(now);
      commands.forEach((retry) => service.publish(retry));
      if (lost.length > 0) {
        setFailed(lost);
        render();
      }
    }, FLUSH_INTERVAL_MS);

    return () => {
      window.clearInterval(timer);
      service.disconnect();
      serviceRef.current = null;
      setBroker("idle");
    };
  }, [deviceId, render]);

  const set = useCallback(
    (patch: SyncPatch) => {
      engineRef.current.set(patch);
      setFailed([]);
      render();
    },
    [render],
  );

  const sendOnce = useCallback((command: LauncherCommand) => {
    serviceRef.current?.publish(command);
  }, []);

  return { state, pending, failed, deviceOnline, broker, ota, set, sendOnce };
}
