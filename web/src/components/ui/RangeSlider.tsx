import { useId, type CSSProperties } from "react";

/**
 * Props for RangeSlider.
 */
interface RangeSliderProps {
  label: string;
  value: number;
  min: number;
  max: number;
  step?: number;
  disabled?: boolean;
  syncing?: boolean;
  format?: (value: number) => string;
  onChange: (value: number) => void;
}

/**
 * Controlled range input; `syncing` marks a value the device has not confirmed yet.
 */
export function RangeSlider({
  label,
  value,
  min,
  max,
  step = 1,
  disabled = false,
  syncing = false,
  format = (v) => `${v}`,
  onChange,
}: RangeSliderProps) {
  const id = useId();
  const pct = ((value - min) / (max - min)) * 100;

  return (
    <div>
      <div className="mb-1 flex items-baseline justify-between">
        <label htmlFor={id} className="text-[14px] text-ink">
          {label}
        </label>
        <span className="flex items-center gap-1.5 font-mono text-[15px] font-semibold tabular-nums">
          {syncing && <span className="h-1.5 w-1.5 animate-pulse rounded-full bg-ball" title="Đang chờ máy xác nhận" />}
          {format(value)}
        </span>
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
        onChange={(e) => onChange(Number(e.target.value))}
      />
    </div>
  );
}
