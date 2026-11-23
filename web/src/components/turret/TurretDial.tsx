import { LIMITS } from "../../config/limits";
import type { TurretMode } from "../../types/launcher";

/**
 * Props for TurretDial.
 */
interface TurretDialProps {
  mode: TurretMode;
  range: number;
  angle: number;
}

const CX = 100;
const CY = 100;
const R = 84;

function point(deg: number, radius = R): [number, number] {
  const rad = (deg * Math.PI) / 180;
  return [CX + radius * Math.sin(rad), CY - radius * Math.cos(rad)];
}

function arc(from: number, to: number, radius = R): string {
  const [x1, y1] = point(from, radius);
  const [x2, y2] = point(to, radius);
  const large = Math.abs(to - from) > 180 ? 1 : 0;
  return `M ${x1} ${y1} A ${radius} ${radius} 0 ${large} 1 ${x2} ${y2}`;
}

function wedge(range: number): string {
  const [x1, y1] = point(-range);
  const [x2, y2] = point(range);
  return `M ${CX} ${CY} L ${x1} ${y1} A ${R} ${R} 0 0 1 ${x2} ${y2} Z`;
}

/**
 * Top-down sketch of the barrel's aim: the shaded wedge is the sweep/random
 * range, the needle is the fixed angle.
 */
export function TurretDial({ mode, range, angle }: TurretDialProps) {
  const limit = LIMITS.turretAngle.max;
  const fixed = mode === "fixed";
  const [nx, ny] = point(fixed ? angle : 0, R - 6);
  const [lx, ly] = point(-limit, R + 10);
  const [rx, ry] = point(limit, R + 10);

  return (
    <svg viewBox="-12 0 224 112" className="mx-auto block w-full max-w-[280px]" aria-hidden="true">
      <path d={arc(-limit, limit)} fill="none" stroke="var(--color-line)" strokeWidth="1.5" />
      {!fixed && (
        <path
          d={wedge(range)}
          fill="var(--color-ball)"
          fillOpacity="0.16"
          stroke="var(--color-ball)"
          strokeWidth="1"
        />
      )}
      <line
        x1={CX}
        y1={CY}
        x2={nx}
        y2={ny}
        stroke="var(--color-ink)"
        strokeWidth={fixed ? 3 : 1.5}
        strokeDasharray={fixed ? undefined : "3 3"}
        strokeLinecap="round"
      />
      <circle cx={CX} cy={CY} r="7" fill="var(--color-card)" stroke="var(--color-ink)" strokeWidth="1.5" />
      <text x={lx} y={ly} textAnchor="middle" fontSize="9" fill="var(--color-mute)">
        −{limit}°
      </text>
      <text x={rx} y={ry} textAnchor="middle" fontSize="9" fill="var(--color-mute)">
        +{limit}°
      </text>
    </svg>
  );
}
