import { useState, type FormEvent } from "react";

import { normalizeDeviceId } from "../../hooks/useDeviceId";

/**
 * Props for DeviceSetup.
 */
interface DeviceSetupProps {
  initial?: string | null;
  onSubmit: (deviceId: string) => void;
}

/**
 * First-run screen asking for the 6-character device id shown by the WiFi portal.
 */
export function DeviceSetup({ initial, onSubmit }: DeviceSetupProps) {
  const [text, setText] = useState(initial ?? "");
  const [error, setError] = useState(false);

  const submit = (event: FormEvent) => {
    event.preventDefault();
    const id = normalizeDeviceId(text);
    if (!id) {
      setError(true);
      return;
    }
    onSubmit(id);
  };

  return (
    <main className="mx-auto max-w-[420px] px-5 pt-16">
      <div className="flex items-center gap-2.5 font-semibold">
        <span className="h-3.5 w-3.5 rounded-full bg-ball" />
        Máy bắn bóng bàn
      </div>
      <h1 className="mt-6 text-[22px] font-semibold">Chọn máy</h1>
      <p className="mt-1 text-mute">
        Nhập mã thiết bị 6 ký tự hiện ở trang cài WiFi, hoặc tên điểm phát <span className="font-mono">PingPong-XXXXXX</span>.
      </p>
      <form onSubmit={submit} className="mt-6 space-y-3">
        <input
          value={text}
          onChange={(e) => {
            setText(e.target.value);
            setError(false);
          }}
          placeholder="VD: A1B2C3"
          maxLength={6}
          autoCapitalize="characters"
          autoComplete="off"
          spellCheck={false}
          className="w-full rounded-lg border border-line bg-white px-3 py-3 font-mono text-[18px] uppercase tracking-[0.2em] outline-none focus:border-ink"
        />
        {error && <p className="text-[13px] text-bad">Mã gồm 6 ký tự 0–9 và A–F.</p>}
        <button type="submit" className="w-full rounded-lg bg-ink py-3 font-semibold text-card">
          Kết nối
        </button>
      </form>
    </main>
  );
}
