import { LIMITS } from "../../config/limits";
import type { LauncherCommand, LauncherState } from "../../types/launcher";
import { Panel } from "../ui/Panel";
import { RangeSlider } from "../ui/RangeSlider";

/**
 * Props for FeedPanel.
 */
interface FeedPanelProps {
  state: LauncherState;
  disabled: boolean;
  send: (command: LauncherCommand) => void;
}

/**
 * Ball feed rate control.
 */
export function FeedPanel({ state, disabled, send }: FeedPanelProps) {
  const interval = (60 / state.feed).toFixed(1);

  return (
    <Panel title="Tốc độ lên bóng" hint={`Khoảng ${interval} giây một quả.`}>
      <RangeSlider
        label="Số quả mỗi phút"
        remote={state.feed}
        min={LIMITS.feed.min}
        max={LIMITS.feed.max}
        disabled={disabled}
        format={(v) => `${v} quả/phút`}
        onSend={(feed) => send({ feed })}
      />
    </Panel>
  );
}
