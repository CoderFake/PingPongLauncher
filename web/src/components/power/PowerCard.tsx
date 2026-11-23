import type { LauncherCommand, LauncherState } from "../../types/launcher";

/**
 * Props for PowerCard.
 */
interface PowerCardProps {
  state: LauncherState;
  disabled: boolean;
  send: (command: LauncherCommand) => void;
}

function statusText(state: LauncherState): string {
  if (!state.power) return "Đang tắt";
  if (!state.ready) return "Đang lên tốc bánh đà…";
  return "Đang bắn";
}

/**
 * Master on/off switch with run status and shot counter.
 */
export function PowerCard({ state, disabled, send }: PowerCardProps) {
  const on = state.power;

  return (
    <section className="flex items-center gap-4 rounded-xl border border-line bg-card p-4">
      <button
        type="button"
        disabled={disabled}
        aria-pressed={on}
        aria-label={on ? "Tắt máy" : "Bật máy"}
        onClick={() => send({ power: !on })}
        className={`grid h-16 w-16 shrink-0 place-items-center rounded-full border-2 transition-colors disabled:opacity-45 ${
          on ? "border-ink bg-ball text-card" : "border-ink bg-card text-ink"
        }`}
      >
        <svg viewBox="0 0 24 24" className="h-7 w-7" fill="none" stroke="currentColor" strokeWidth="2.2" strokeLinecap="round">
          <path d="M12 3v8" />
          <path d="M6.3 6.8a8 8 0 1 0 11.4 0" />
        </svg>
      </button>
      <div className="min-w-0 flex-1">
        <p className="text-[13px] text-mute">Nguồn</p>
        <p className="text-[18px] font-semibold">{statusText(state)}</p>
      </div>
      <div className="text-right">
        <p className="text-[13px] text-mute">Đã bắn</p>
        <p className="font-mono text-[22px] font-semibold tabular-nums">{state.shots}</p>
      </div>
    </section>
  );
}
