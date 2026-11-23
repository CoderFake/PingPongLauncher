/**
 * Returns a short random id used by the firmware to drop duplicate deliveries.
 */
export function makeCommandId(): string {
  const bytes = new Uint8Array(8);
  crypto.getRandomValues(bytes);
  return Array.from(bytes, (b) => b.toString(36).padStart(2, "0")).join("").slice(0, 14);
}
