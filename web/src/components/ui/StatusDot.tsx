/**
 * Props for StatusDot.
 */
interface StatusDotProps {
  tone: "ok" | "wait" | "off";
  label: string;
}

const TONES = {
  ok: "bg-table",
  wait: "bg-ball animate-pulse",
  off: "bg-line",
} as const;

/**
 * Small colored dot followed by a text label.
 */
export function StatusDot({ tone, label }: StatusDotProps) {
  return (
    <span className="inline-flex items-center gap-1.5 text-[13px] text-mute">
      <span className={`h-2 w-2 rounded-full ${TONES[tone]}`} />
      {label}
    </span>
  );
}
