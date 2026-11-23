/**
 * One option of a SegmentedControl.
 */
export interface SegmentOption<T extends string> {
  value: T;
  label: string;
}

/**
 * Props for SegmentedControl.
 */
interface SegmentedControlProps<T extends string> {
  options: SegmentOption<T>[];
  value: T;
  disabled?: boolean;
  onChange: (value: T) => void;
}

/**
 * Row of mutually exclusive buttons.
 */
export function SegmentedControl<T extends string>({
  options,
  value,
  disabled = false,
  onChange,
}: SegmentedControlProps<T>) {
  return (
    <div className="grid auto-cols-fr grid-flow-col rounded-lg border border-line bg-paper p-1">
      {options.map((option) => {
        const active = option.value === value;
        return (
          <button
            key={option.value}
            type="button"
            disabled={disabled}
            onClick={() => !active && onChange(option.value)}
            className={`rounded-md px-2 py-2 text-[14px] transition-colors disabled:opacity-45 ${
              active ? "bg-ink font-semibold text-card" : "text-mute hover:text-ink"
            }`}
          >
            {option.label}
          </button>
        );
      })}
    </div>
  );
}
