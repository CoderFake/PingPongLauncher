/**
 * Compares dotted versions such as "1.2.0" and "1.10.1"; returns -1, 0 or 1.
 */
export function compareVersions(a: string, b: string): number {
  const pa = a.split(/[.-]/).map((x) => parseInt(x, 10) || 0);
  const pb = b.split(/[.-]/).map((x) => parseInt(x, 10) || 0);
  for (let i = 0; i < Math.max(pa.length, pb.length); i++) {
    const diff = (pa[i] ?? 0) - (pb[i] ?? 0);
    if (diff !== 0) {
      return diff > 0 ? 1 : -1;
    }
  }
  return 0;
}
