import type { ReactNode } from "react";

/**
 * Props for Panel.
 */
interface PanelProps {
  title: string;
  hint?: string;
  aside?: ReactNode;
  children: ReactNode;
}

/**
 * Bordered card section with a small header row.
 */
export function Panel({ title, hint, aside, children }: PanelProps) {
  return (
    <section className="rounded-xl border border-line bg-card">
      <header className="flex items-start justify-between gap-3 border-b border-line px-4 py-3">
        <div>
          <h2 className="text-[15px] font-semibold">{title}</h2>
          {hint && <p className="mt-0.5 text-[13px] text-mute">{hint}</p>}
        </div>
        {aside}
      </header>
      <div className="space-y-5 px-4 py-4">{children}</div>
    </section>
  );
}
