import { useState } from "react";

import { LOCAL_MODE } from "./config/transport";
import { FeedPanel } from "./components/feed/FeedPanel";
import { FirmwarePanel } from "./components/firmware/FirmwarePanel";
import { FlywheelPanel } from "./components/flywheel/FlywheelPanel";
import { DeviceSetup } from "./components/layout/DeviceSetup";
import { TopBar } from "./components/layout/TopBar";
import { PowerCard } from "./components/power/PowerCard";
import { TurretPanel } from "./components/turret/TurretPanel";
import { useDeviceId } from "./hooks/useDeviceId";
import { useLauncher } from "./hooks/useLauncher";
import { useLocalDeviceId } from "./hooks/useLocalDeviceId";

/**
 * Root layout: device picker until an id is set, then the control panels.
 */
export default function App() {
  const [storedId, setDeviceId] = useDeviceId();
  const localId = useLocalDeviceId();
  const deviceId = LOCAL_MODE ? localId : storedId;
  const [editing, setEditing] = useState(false);
  const { state, pending, failed, deviceOnline, broker, ota, set, sendOnce } = useLauncher(editing ? null : deviceId);

  if (LOCAL_MODE && !deviceId) {
    return (
      <main className="mx-auto max-w-[460px] px-4 pt-16 text-center text-mute">Đang kết nối tới máy…</main>
    );
  }

  if (!deviceId || editing) {
    return (
      <DeviceSetup
        initial={deviceId}
        onSubmit={(id) => {
          setDeviceId(id);
          setEditing(false);
        }}
      />
    );
  }

  const disabled = !state || !deviceOnline || broker !== "connected";

  return (
    <main className="mx-auto max-w-[460px] px-4 pb-12 pt-6">
      <TopBar
        deviceId={deviceId}
        broker={broker}
        deviceOnline={deviceOnline}
        onChangeDevice={() => setEditing(true)}
      />
      {!state ? (
        <div className="rounded-xl border border-dashed border-line px-4 py-10 text-center text-mute">
          Đang chờ dữ liệu từ máy {deviceId}…
          <p className="mt-2 text-[13px]">Kiểm tra máy đã bật và đã kết nối WiFi.</p>
        </div>
      ) : (
        <div className="space-y-4">
          {failed.length > 0 && (
            <p className="rounded-lg bg-bad/10 px-3 py-2 text-[14px] text-bad">
              Máy không xác nhận thay đổi sau 3 lần gửi. Kiểm tra máy còn online rồi chỉnh lại.
            </p>
          )}
          <PowerCard state={state} disabled={disabled} set={set} pending={pending} />
          <FlywheelPanel state={state} disabled={disabled} set={set} pending={pending} />
          <TurretPanel state={state} disabled={disabled} set={set} pending={pending} />
          <FeedPanel state={state} disabled={disabled} set={set} pending={pending} />
          {!LOCAL_MODE && <FirmwarePanel state={state} ota={ota} disabled={disabled} send={sendOnce} />}
        </div>
      )}
    </main>
  );
}
