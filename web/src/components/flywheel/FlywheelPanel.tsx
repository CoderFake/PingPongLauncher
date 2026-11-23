import { LIMITS } from "../../config/limits";
import type { LauncherState, SyncKey, SyncPatch } from "../../types/launcher";
import { Panel } from "../ui/Panel";
import { RangeSlider } from "../ui/RangeSlider";

/**
 * Props for FlywheelPanel.
 */
interface FlywheelPanelProps {
  state: LauncherState;
  disabled: boolean;
  set: (patch: SyncPatch) => void;
  pending: Set<SyncKey>;
}

const percent = (v: number) => `${v}%`;

/**
 * Speed controls for the upper (SB1) and lower (SB2) flywheels plus a linked slider.
 */
export function FlywheelPanel({ state, disabled, set, pending }: FlywheelPanelProps) {
  const linked = state.sb1 === state.sb2 ? state.sb1 : Math.round((state.sb1 + state.sb2) / 2);
  const spin = state.sb1 - state.sb2;
  const spinLabel = spin === 0 ? "Không xoáy" : spin > 0 ? "Xoáy lên" : "Xoáy xuống";

  return (
    <Panel
      title="Bánh đà"
      hint="Chênh lệch tốc độ giữa hai bánh tạo độ xoáy."
      aside={
        <span className="whitespace-nowrap rounded border border-line px-2 py-0.5 text-[12px] text-mute">
          {spinLabel}
        </span>
      }
    >
      <RangeSlider
        label="SB1 · bánh trên"
        value={state.sb1}
        min={LIMITS.speed.min}
        max={LIMITS.speed.max}
        disabled={disabled}
        format={percent}
        syncing={pending.has("sb1")}
        onChange={(sb1) => set({ sb1 })}
      />
      <RangeSlider
        label="SB2 · bánh dưới"
        value={state.sb2}
        min={LIMITS.speed.min}
        max={LIMITS.speed.max}
        disabled={disabled}
        format={percent}
        syncing={pending.has("sb2")}
        onChange={(sb2) => set({ sb2 })}
      />
      <div className="border-t border-dashed border-line pt-4">
        <RangeSlider
          label="Chỉnh chung cả hai"
          value={linked}
          min={LIMITS.speed.min}
          max={LIMITS.speed.max}
          disabled={disabled}
          format={percent}
          syncing={pending.has("sb1") || pending.has("sb2")}
          onChange={(sbAll) => set({ sb1: sbAll, sb2: sbAll })}
        />
      </div>
    </Panel>
  );
}
