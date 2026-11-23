import { useId, type CSSProperties } from "react";

import { useSyncedValue } from "../../hooks/useSyncedValue";

/**
 * Props for RangeSlider.
 */
interface RangeSliderProps {
  label: string;
  remote: number;
  min: number;
  max: number;
  step?: number;
  disabled?: boolean;
  format?: (value: number) => string;
  onSend: (value: number) => void;
}

/**
 * Labeled range input synced with a remote value through useSyncedValue.
 */
export function RangeSlider({
  label,
  remote,
  min,
  max,
  step = 1,
  disabled = false,
  format = (v) => `${v}`,
  onSend,
}: RangeSliderProps) {
  const id = useId();
  const { value, change, commit } = useSyncedValue(remote, onSend);
  const pct = ((value - min) / (max - min)) * 100;

  return (
    <div>
      <div className="mb-1 flex items-baseline justify-between">
        <label htmlFor={id} className="text-[14px] text-ink">
          {label}
        </label>
        <span className="font-mono text-[15px] font-semibold tabular-nums">{format(value)}</span>
      </div>
      <input
        id={id}
        type="range"
        className="range"
        min={min}
        max={max}
        step={step}
        value={value}
        disabled={disabled}
        style={{ "--pct": `${pct}%` } as CSSProperties}
        onChange={(e) => change(Number(e.target.value))}
        onPointerUp={commit}
        onKeyUp={commit}
        onBlur={commit}
      />
    </div>
  );
}
