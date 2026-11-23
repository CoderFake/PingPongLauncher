import { useLatestFirmware } from "../../hooks/useLatestFirmware";
import type { LauncherCommand, LauncherState, OtaStatus } from "../../types/launcher";
import { compareVersions } from "../../utils/version";
import { Panel } from "../ui/Panel";

/**
 * Props for FirmwarePanel.
 */
interface FirmwarePanelProps {
  state: LauncherState;
  ota: OtaStatus | null;
  disabled: boolean;
  send: (command: LauncherCommand) => void;
}

const OTA_ERRORS: Record<string, string> = {
  power_on: "Tắt nguồn máy trước khi cập nhật.",
  url_not_allowed: "Link firmware không thuộc kho được phép.",
  bad_sha256: "Thiếu mã kiểm tra SHA-256.",
  same_version: "Máy đang chạy đúng phiên bản này.",
  download_timeout: "Tải firmware bị gián đoạn.",
  sha_mismatch: "File tải về không khớp mã kiểm tra, đã hủy.",
  no_space: "Không đủ bộ nhớ. Kiểm tra Partition Scheme.",
  flash_write: "Lỗi ghi bộ nhớ flash.",
};

/**
 * Turns a firmware OTA error code into a Vietnamese message.
 */
function otaErrorText(code: string | undefined): string {
  if (code && OTA_ERRORS[code]) return OTA_ERRORS[code];
  if (code?.startsWith("http_")) return `Không tải được file firmware (mã ${code.slice(5)}).`;
  return `Cập nhật lỗi: ${code ?? "không rõ"}`;
}

const RELEASE_ERRORS: Record<string, string> = {
  no_release: "Chưa có bản phát hành nào.",
  no_asset: "Bản phát hành không có file .bin.",
  no_digest: "GitHub chưa trả mã SHA-256 cho file.",
};

/**
 * Shows the running firmware version and installs the latest GitHub release over the air.
 */
export function FirmwarePanel({ state, ota, disabled, send }: FirmwarePanelProps) {
  const { release, error, loading, reload } = useLatestFirmware();
  const current = state.fw ?? "?";
  const running = ota !== null && (ota.state === "queued" || ota.state === "downloading" || ota.state === "rebooting");
  const newer = release !== null && compareVersions(release.version, current) > 0;
  const canUpdate = newer && !disabled && !state.power && !running;

  const install = () => {
    if (!release) return;
    send({ ota: { url: release.url, sha256: release.sha256, version: release.version } });
  };

  let hint = "Đang kiểm tra bản mới…";
  if (error) hint = RELEASE_ERRORS[error] ?? "Không kiểm tra được bản mới.";
  else if (release && !newer) hint = "Máy đang chạy bản mới nhất.";
  else if (release && newer) hint = state.power ? "Có bản mới. Tắt nguồn máy để cập nhật." : "Có bản mới.";

  return (
    <Panel
      title="Firmware"
      hint={hint}
      aside={
        <button
          type="button"
          onClick={reload}
          disabled={loading}
          className="rounded-md border border-line px-2.5 py-1 text-[13px] text-mute hover:text-ink disabled:opacity-45"
        >
          Kiểm tra
        </button>
      }
    >
      <dl className="grid grid-cols-2 gap-3 text-[14px]">
        <div>
          <dt className="text-mute">Đang chạy</dt>
          <dd className="font-mono text-[16px] font-semibold">v{current}</dd>
        </div>
        <div>
          <dt className="text-mute">Mới nhất</dt>
          <dd className="font-mono text-[16px] font-semibold">{release ? `v${release.version}` : "—"}</dd>
        </div>
      </dl>

      {running && ota && (
        <div>
          <div className="mb-1 flex justify-between text-[13px] text-mute">
            <span>{ota.state === "rebooting" ? "Đang khởi động lại…" : `Đang cài v${ota.version}`}</span>
            <span className="font-mono">{ota.progress}%</span>
          </div>
          <div className="h-1.5 overflow-hidden rounded-full bg-line">
            <div className="h-full bg-ball transition-[width]" style={{ width: `${ota.progress}%` }} />
          </div>
        </div>
      )}

      {ota?.state === "error" && (
        <p className="rounded-lg bg-bad/10 px-3 py-2 text-[14px] text-bad">
          {otaErrorText(ota.error)}
        </p>
      )}

      {newer && (
        <button
          type="button"
          disabled={!canUpdate}
          onClick={install}
          className="w-full rounded-lg bg-ink py-3 text-[15px] font-semibold text-card disabled:opacity-45"
        >
          Cập nhật lên v{release.version}
        </button>
      )}
    </Panel>
  );
}
