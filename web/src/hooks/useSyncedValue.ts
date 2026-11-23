import { useCallback, useEffect, useRef, useState } from "react";

import { ECHO_TIMEOUT_MS, SEND_INTERVAL_MS } from "../config/limits";

/**
 * Value returned by useSyncedValue.
 */
export interface SyncedValue {
  value: number;
  change: (next: number) => void;
  commit: () => void;
}

/**
 * Bridges a slider and a remote value: keeps a local draft while dragging,
 * throttles outgoing sends, never resends an unchanged value, and releases
 * the draft once the device echoes it back (or after a timeout).
 */
export function useSyncedValue(remote: number, send: (value: number) => void): SyncedValue {
  const [draft, setDraft] = useState<number | null>(null);
  const draftRef = useRef<number | null>(null);
  const lastSentRef = useRef<number>(remote);
  const pendingRef = useRef<number | null>(null);
  const timerRef = useRef<number | null>(null);
  const releaseRef = useRef<number | null>(null);
  const lastSendAtRef = useRef(0);
  const sendRef = useRef(send);
  sendRef.current = send;

  const setDraftValue = useCallback((value: number | null) => {
    draftRef.current = value;
    setDraft(value);
  }, []);

  const clearTimer = (ref: { current: number | null }) => {
    if (ref.current !== null) {
      window.clearTimeout(ref.current);
      ref.current = null;
    }
  };

  const dispatch = useCallback((value: number) => {
    pendingRef.current = null;
    if (value === lastSentRef.current) {
      return;
    }
    lastSentRef.current = value;
    lastSendAtRef.current = Date.now();
    sendRef.current(value);
  }, []);

  const change = useCallback(
    (next: number) => {
      setDraftValue(next);
      clearTimer(releaseRef);
      pendingRef.current = next;
      const wait = SEND_INTERVAL_MS - (Date.now() - lastSendAtRef.current);
      if (wait <= 0) {
        clearTimer(timerRef);
        dispatch(next);
        return;
      }
      if (timerRef.current === null) {
        timerRef.current = window.setTimeout(() => {
          timerRef.current = null;
          if (pendingRef.current !== null) {
            dispatch(pendingRef.current);
          }
        }, wait);
      }
    },
    [dispatch, setDraftValue],
  );

  const commit = useCallback(() => {
    clearTimer(timerRef);
    if (pendingRef.current !== null) {
      dispatch(pendingRef.current);
    }
    clearTimer(releaseRef);
    releaseRef.current = window.setTimeout(() => {
      releaseRef.current = null;
      setDraftValue(null);
    }, ECHO_TIMEOUT_MS);
  }, [dispatch, setDraftValue]);

  useEffect(() => {
    if (draftRef.current === null) {
      lastSentRef.current = remote;
      return;
    }
    const settled = timerRef.current === null && pendingRef.current === null;
    if (settled && remote === draftRef.current) {
      clearTimer(releaseRef);
      setDraftValue(null);
    }
  }, [remote, setDraftValue]);

  useEffect(
    () => () => {
      clearTimer(timerRef);
      clearTimer(releaseRef);
    },
    [],
  );

  return { value: draft ?? remote, change, commit };
}
