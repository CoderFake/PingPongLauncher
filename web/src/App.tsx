import { useState } from "react";

import { FeedPanel } from "./components/feed/FeedPanel";
import { FirmwarePanel } from "./components/firmware/FirmwarePanel";
import { FlywheelPanel } from "./components/flywheel/FlywheelPanel";
import { DeviceSetup } from "./components/layout/DeviceSetup";
import { TopBar } from "./components/layout/TopBar";
import { PowerCard } from "./components/power/PowerCard";
import { TurretPanel } from "./components/turret/TurretPanel";
import { useDeviceId } from "./hooks/useDeviceId";
import { useLauncher } from "./hooks/useLauncher";

/**
 * Root layout: device picker until an id is set, then the control panels.
 */
export default function App() {
  const [deviceId, setDeviceId] = useDeviceId();
  const [editing, setEditing] = useState(false);
  const { state, deviceOnline, broker, ota, send } = useLauncher(editing ? null : deviceId);

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
          <PowerCard state={state} disabled={disabled} send={send} />
          <FlywheelPanel state={state} disabled={disabled} send={send} />
          <TurretPanel state={state} disabled={disabled} send={send} />
          <FeedPanel state={state} disabled={disabled} send={send} />
          <FirmwarePanel state={state} ota={ota} disabled={disabled} send={send} />
        </div>
      )}
    </main>
  );
}
