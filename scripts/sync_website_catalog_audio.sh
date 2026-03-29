#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEST_DIR="${ROOT_DIR}/apps/website/assets/audio/catalog"
MAX_ASSET_BYTES=$((24 * 1024 * 1024))
CATALOG_JSONS=(
  "${ROOT_DIR}/apps/website/assets/data/beat_catalog.json"
  "${ROOT_DIR}/server/storage/catalog.json"
)

SOURCE_CANDIDATES=(
  "${ROOT_DIR}/server/data/North3rnlighter beats"
  "${ROOT_DIR}/server/data/North3rnlight3r_Beatz"
  "${HOME:-/home/north3rnlight3r}/Music/North3rnlight3r_Beatz"
  "${HOME:-/home/north3rnlight3r}/Music/North3rnlighter beats"
)

update_catalog_jsons() {
  local asset_dir="$1"
  python3 - "$asset_dir" "${CATALOG_JSONS[@]}" <<'PY'
import json
import sys
from pathlib import Path
from urllib.parse import quote

asset_dir = Path(sys.argv[1])
catalog_paths = [Path(value) for value in sys.argv[2:]]
asset_names = {path.name for path in asset_dir.iterdir() if path.is_file()}

def resolve_asset_name(file_name: str) -> str:
    safe_name = Path(str(file_name or "")).name
    if not safe_name:
        return ""
    if safe_name in asset_names:
        return safe_name
    stem = Path(safe_name).stem
    direct_matches = sorted(
        name for name in asset_names
        if Path(name).stem == stem and ".preview" not in Path(name).stem
    )
    if direct_matches:
        return direct_matches[0]
    preview_name = f"{stem}.preview.mp3"
    if preview_name in asset_names:
        return preview_name
    return ""

for catalog_path in catalog_paths:
    if not catalog_path.exists():
        continue
    try:
        payload = json.loads(catalog_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError:
        continue
    if not isinstance(payload, list):
        continue

    changed = False
    for entry in payload:
        if not isinstance(entry, dict):
            continue
        file_name = Path(str(entry.get("file_name") or "")).name
        if not file_name:
            continue
        asset_file_name = resolve_asset_name(file_name)
        public_target = asset_file_name or file_name
        public_url = f"/assets/audio/catalog/{quote(public_target)}"
        if entry.get("asset_file_name") != asset_file_name:
            entry["asset_file_name"] = asset_file_name
            changed = True
        if entry.get("public_url") != public_url:
            entry["public_url"] = public_url
            changed = True
        if entry.get("full_song_url") != public_url:
            entry["full_song_url"] = public_url
            changed = True
        if entry.get("stream_url") != public_url:
            entry["stream_url"] = public_url
            changed = True
    if changed:
        catalog_path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
        print(f"Updated catalog metadata: {catalog_path}")
PY
}

mkdir -p "${DEST_DIR}"
find "${DEST_DIR}" -maxdepth 1 -type f ! -name '.gitkeep' -delete

for source_dir in "${SOURCE_CANDIDATES[@]}"; do
  if [[ ! -d "${source_dir}" ]]; then
    continue
  fi

  mapfile -d '' audio_files < <(find "${source_dir}" -maxdepth 1 -type f \( -iname '*.wav' -o -iname '*.mp3' -o -iname '*.flac' -o -iname '*.m4a' -o -iname '*.ogg' -o -iname '*.aiff' -o -iname '*.aif' \) -print0)
  file_count="${#audio_files[@]}"
  if [[ "${file_count}" -eq 0 ]]; then
    continue
  fi

  for source_file in "${audio_files[@]}"; do
    base_name="$(basename "${source_file}")"
    file_size="$(stat -c '%s' "${source_file}")"
    if [[ "${file_size}" -le "${MAX_ASSET_BYTES}" && "${base_name,,}" == *.mp3 ]]; then
      cp -f "${source_file}" "${DEST_DIR}/${base_name}"
      continue
    fi

    asset_name="$(basename "${source_file%.*}").mp3"
    ffmpeg -y -v error -i "${source_file}" -vn -map_metadata -1 \
      -ac 2 -ar 44100 -c:a libmp3lame -b:a 192k \
      "${DEST_DIR}/${asset_name}"
  done

  update_catalog_jsons "${DEST_DIR}"
  echo "Synced ${file_count} catalog audio files from ${source_dir} to ${DEST_DIR}"
  exit 0
done

echo "No catalog audio files found in configured source directories." >&2
exit 1
