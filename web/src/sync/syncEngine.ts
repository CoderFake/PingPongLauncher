import {
  ACK_TIMEOUT_MS,
  MAX_SEND_ATTEMPTS,
  STATE_AFTER_ACK_TIMEOUT_MS,
} from "../config/limits";
import type { AckMessage, LauncherCommand, LauncherState, SyncKey, SyncPatch } from "../types/launcher";
import { makeCommandId } from "../utils/commandId";

/**
 * Command ready to publish, always carrying its id.
 */
export interface OutgoingCommand extends LauncherCommand {
  id: string;
}

/**
 * Work produced by one engine tick.
 */
export interface TickResult {
  commands: OutgoingCommand[];
  failed: SyncKey[];
}

type SyncValue = LauncherState[SyncKey];

interface PendingEdit {
  value: SyncValue;
  dirty: boolean;
  id: string | null;
  sentAt: number;
  attempts: number;
  ackRev: number | null;
  ackBoot: number | null;
  ackAt: number;
}

/**
 * Optimistic two-way sync between the UI and the device.
 *
 * - The device is the source of truth (`confirmed`); local edits live in `pending`
 *   and are shown on top of it, so the UI never jumps back while a change is in flight.
 * - Dirty edits are batched into one command per flush; each command has a unique id.
 * - An edit is settled only when the device acknowledged its command id AND a state
 *   with rev >= the acked rev (same boot) has arrived. Unacked commands are resent with
 *   the same id, which the firmware deduplicates, so retries are idempotent.
 * - States older than the newest one seen in the same boot are ignored.
 */
export class SyncEngine {
  private confirmed: LauncherState | null = null;
  private pending = new Map<SyncKey, PendingEdit>();
  private sent = new Map<string, OutgoingCommand>();

  /**
   * Forgets everything, e.g. when switching to another device.
   */
  reset(): void {
    this.confirmed = null;
    this.pending.clear();
    this.sent.clear();
  }

  /**
   * Records local edits; they are sent on the next flush.
   */
  set(patch: SyncPatch): void {
    for (const key of Object.keys(patch) as SyncKey[]) {
      const value = patch[key];
      if (value === undefined) continue;
      const current = this.pending.get(key);
      if (current ? current.value === value : this.confirmed?.[key] === value) continue;
      this.pending.set(key, {
        value,
        dirty: true,
        id: null,
        sentAt: 0,
        attempts: 0,
        ackRev: null,
        ackBoot: null,
        ackAt: 0,
      });
    }
  }

  /**
   * Batches all dirty edits into one command, or returns null when nothing changed.
   */
  flush(now: number): OutgoingCommand | null {
    const dirty = [...this.pending].filter(([, edit]) => edit.dirty);
    if (dirty.length === 0) return null;

    const command: OutgoingCommand = { id: makeCommandId() };
    const fields = command as unknown as Record<string, SyncValue>;
    for (const [key, edit] of dirty) {
      fields[key] = edit.value;
      edit.dirty = false;
      edit.id = command.id;
      edit.sentAt = now;
      edit.attempts = 1;
    }
    this.sent.set(command.id, command);
    return command;
  }

  /**
   * Resends unacknowledged commands and gives up on edits that never settle.
   */
  tick(now: number): TickResult {
    const commands: OutgoingCommand[] = [];
    const failed: SyncKey[] = [];
    const resent = new Set<string>();

    for (const [key, edit] of this.pending) {
      if (edit.dirty || edit.id === null) continue;

      if (edit.ackRev !== null) {
        if (now - edit.ackAt > STATE_AFTER_ACK_TIMEOUT_MS) this.pending.delete(key);
        continue;
      }
      if (now - edit.sentAt < ACK_TIMEOUT_MS) continue;

      if (edit.attempts >= MAX_SEND_ATTEMPTS) {
        this.pending.delete(key);
        failed.push(key);
        continue;
      }
      edit.attempts += 1;
      edit.sentAt = now;
      const command = this.sent.get(edit.id);
      if (command && !resent.has(edit.id)) {
        resent.add(edit.id);
        commands.push(command);
      }
    }
    this.collectSent();
    return { commands, failed };
  }

  /**
   * Applies a device acknowledgement.
   */
  onAck(ack: AckMessage, now: number): void {
    for (const edit of this.pending.values()) {
      if (edit.id === ack.id && edit.ackRev === null) {
        edit.ackRev = ack.rev;
        edit.ackBoot = ack.boot;
        edit.ackAt = now;
      }
    }
    this.settle();
  }

  /**
   * Applies a device state; returns false when it was stale and ignored.
   */
  onState(state: LauncherState): boolean {
    const current = this.confirmed;
    if (current && current.boot === state.boot && state.rev < current.rev) return false;
    this.confirmed = state;
    this.settle();
    return true;
  }

  /**
   * Device state with in-flight local edits laid on top.
   */
  view(): LauncherState | null {
    if (!this.confirmed) return null;
    const merged = { ...this.confirmed } as Record<string, unknown>;
    for (const [key, edit] of this.pending) merged[key] = edit.value;
    return merged as unknown as LauncherState;
  }

  /**
   * Keys that still wait for the device.
   */
  pendingKeys(): Set<SyncKey> {
    return new Set(this.pending.keys());
  }

  private settle(): void {
    const state = this.confirmed;
    if (!state) return;
    for (const [key, edit] of this.pending) {
      if (edit.ackRev === null) continue;
      if (state.boot !== edit.ackBoot || state.rev >= edit.ackRev) this.pending.delete(key);
    }
    this.collectSent();
  }

  private collectSent(): void {
    const live = new Set<string>();
    for (const edit of this.pending.values()) if (edit.id) live.add(edit.id);
    for (const id of this.sent.keys()) if (!live.has(id)) this.sent.delete(id);
  }
}
