import { GITHUB_REPO } from "../config/github";

/**
 * Firmware image published as a GitHub release asset.
 */
export interface FirmwareRelease {
  version: string;
  url: string;
  sha256: string;
  publishedAt: string;
  notes: string;
}

interface ReleaseAsset {
  name: string;
  browser_download_url: string;
  digest?: string | null;
}

interface ReleaseResponse {
  tag_name: string;
  published_at: string;
  body?: string | null;
  assets: ReleaseAsset[];
}

/**
 * Loads the latest release and picks the app image (*.bin, not merged/bootloader).
 * The SHA-256 comes from GitHub's asset digest, so no extra manifest is needed.
 */
export async function fetchLatestFirmware(): Promise<FirmwareRelease> {
  const response = await fetch(`https://api.github.com/repos/${GITHUB_REPO}/releases/latest`, {
    headers: { Accept: "application/vnd.github+json" },
  });
  if (response.status === 404) {
    throw new Error("no_release");
  }
  if (!response.ok) {
    throw new Error(`github_${response.status}`);
  }
  const release = (await response.json()) as ReleaseResponse;
  const asset = release.assets.find(
    (a) => a.name.endsWith(".bin") && !/merged|bootloader|partitions/i.test(a.name),
  );
  if (!asset) {
    throw new Error("no_asset");
  }
  const sha256 = asset.digest?.startsWith("sha256:") ? asset.digest.slice(7) : "";
  if (!/^[0-9a-f]{64}$/i.test(sha256)) {
    throw new Error("no_digest");
  }
  return {
    version: release.tag_name.replace(/^v/, ""),
    url: asset.browser_download_url,
    sha256,
    publishedAt: release.published_at,
    notes: release.body ?? "",
  };
}
