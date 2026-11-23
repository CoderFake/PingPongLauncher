import { LIMITS } from "../../config/limits";
import type { LauncherState, SyncKey, SyncPatch } from "../../types/launcher";
import { Panel } from "../ui/Panel";
import { RangeSlider } from "../ui/RangeSlider";

/**
 * Props for FeedPanel.
 */
interface FeedPanelProps {
  state: LauncherState;
  disabled: boolean;
  set: (patch: SyncPatch) => void;
  pending: Set<SyncKey>;
}

/**
 * Ball feed rate control.
 */
export function FeedPanel({ state, disabled, set, pending }: FeedPanelProps) {
  const interval = (60 / state.feed).toFixed(1);

  return (
    <Panel title="Tốc độ lên bóng" hint={`Khoảng ${interval} giây một quả.`}>
      <RangeSlider
        label="Số quả mỗi phút"
        value={state.feed}
        min={LIMITS.feed.min}
        max={LIMITS.feed.max}
        disabled={disabled}
        format={(v) => `${v} quả/phút`}
        syncing={pending.has("feed")}
        onChange={(feed) => set({ feed })}
      />
    </Panel>
  );
}
