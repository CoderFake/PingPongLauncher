import { LIMITS } from "../../config/limits";
import type { LauncherCommand, LauncherState, TurretMode } from "../../types/launcher";
import { Panel } from "../ui/Panel";
import { RangeSlider } from "../ui/RangeSlider";
import { SegmentedControl, type SegmentOption } from "../ui/SegmentedControl";
import { TurretDial } from "./TurretDial";

/**
 * Props for TurretPanel.
 */
interface TurretPanelProps {
  state: LauncherState;
  disabled: boolean;
  send: (command: LauncherCommand) => void;
}

const MODES: SegmentOption<TurretMode>[] = [
  { value: "sweep", label: "Quét" },
  { value: "random", label: "Ngẫu nhiên" },
  { value: "fixed", label: "Cố định" },
];

const HINTS: Record<TurretMode, string> = {
  sweep: "Cổ nòng quét qua lại đều trong khoảng góc.",
  random: "Mỗi quả bắn ra một hướng ngẫu nhiên trong khoảng góc.",
  fixed: "Cổ nòng đứng yên ở góc đã chọn. 0° là thẳng giữa.",
};

const signed = (v: number) => (v > 0 ? `+${v}°` : `${v}°`);

/**
 * Aim mode, sweep range and fixed angle controls.
 */
export function TurretPanel({ state, disabled, send }: TurretPanelProps) {
  const fixed = state.mode === "fixed";

  return (
    <Panel title="Hướng bắn" hint={HINTS[state.mode]}>
      <SegmentedControl
        options={MODES}
        value={state.mode}
        disabled={disabled}
        onChange={(mode) => send({ mode })}
      />
      <TurretDial mode={state.mode} range={state.range} angle={state.angle} />
      {fixed ? (
        <RangeSlider
          label="Góc đứng yên"
          remote={state.angle}
          min={LIMITS.turretAngle.min}
          max={LIMITS.turretAngle.max}
          disabled={disabled}
          format={signed}
          onSend={(angle) => send({ angle })}
        />
      ) : (
        <RangeSlider
          label="Khoảng góc"
          remote={state.range}
          min={LIMITS.turretRange.min}
          max={LIMITS.turretRange.max}
          disabled={disabled}
          format={(v) => `±${v}°`}
          onSend={(range) => send({ range })}
        />
      )}
    </Panel>
  );
}
