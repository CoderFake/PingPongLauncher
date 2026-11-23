import type { BrokerStatus } from "../../types/launcher";
import { StatusDot } from "../ui/StatusDot";

/**
 * Props for TopBar.
 */
interface TopBarProps {
  deviceId: string;
  broker: BrokerStatus;
  deviceOnline: boolean;
  onChangeDevice: () => void;
}

const BROKER_LABEL: Record<BrokerStatus, string> = {
  idle: "Chưa kết nối",
  connecting: "Đang nối máy chủ",
  connected: "Máy chủ OK",
  reconnecting: "Đang nối lại",
  offline: "Mất mạng",
};

/**
 * Header with brand, device id and connection indicators.
 */
export function TopBar({ deviceId, broker, deviceOnline, onChangeDevice }: TopBarProps) {
  const brokerTone = broker === "connected" ? "ok" : broker === "offline" ? "off" : "wait";

  return (
    <header className="mb-5">
      <div className="flex items-center justify-between">
        <div className="flex items-center gap-2.5 font-semibold">
          <span className="h-3.5 w-3.5 rounded-full bg-ball shadow-[inset_-3px_-3px_0_rgba(0,0,0,0.12)]" />
          Máy bắn bóng bàn
        </div>
        <button
          type="button"
          onClick={onChangeDevice}
          className="rounded-md border border-line bg-card px-2.5 py-1 font-mono text-[13px] hover:border-ink"
          title="Đổi thiết bị"
        >
          {deviceId}
        </button>
      </div>
      <div className="mt-3 flex flex-wrap gap-x-4 gap-y-1">
        <StatusDot tone={brokerTone} label={BROKER_LABEL[broker]} />
        <StatusDot tone={deviceOnline ? "ok" : "off"} label={deviceOnline ? "Máy đang online" : "Máy offline"} />
      </div>
    </header>
  );
}
