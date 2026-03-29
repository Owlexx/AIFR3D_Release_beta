#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEFAULT_SOURCE_DIRS=(
  "${SOURCE_DIR:-}"
  "$HOME/Music/North3rnlight3r_Beatz"
  "$HOME/Music/North3rnlighter beats"
  "/home/north3rnlight3r/Music/North3rnlight3r_Beatz"
  "/home/north3rnlight3r/Music/North3rnlighter beats"
)
R2_BUCKET="${R2_BUCKET:-}"
R2_PREFIX="${R2_PREFIX:-catalog}"
R2_PUBLIC_BASE_URL="${R2_PUBLIC_BASE_URL:-}"

resolve_source_dir() {
  local candidate=""
  for candidate in "${DEFAULT_SOURCE_DIRS[@]}"; do
    [[ -n "$candidate" ]] || continue
    if [[ -d "$candidate" ]]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done
  return 1
}

SOURCE_DIR="$(resolve_source_dir)"

if [[ -z "$R2_BUCKET" ]]; then
  echo "R2_BUCKET is required"
  exit 1
fi

if [[ -z "$R2_PUBLIC_BASE_URL" ]]; then
  echo "R2_PUBLIC_BASE_URL is required"
  exit 1
fi

command -v wrangler >/dev/null || {
  echo "wrangler not found"
  exit 1
}

cd "$ROOT_DIR"

tmp_json="$(mktemp)"
printf '[]' > "$tmp_json"

while IFS= read -r -d '' file; do
  name="$(basename "$file")"
  key_path="$R2_PREFIX/$name"

  echo "Uploading: $name"
  wrangler r2 object put "$R2_BUCKET/$key_path" --file "$file"

  size="$(stat -c '%s' "$file")"
  mtime="$(stat -c '%y' "$file" | cut -d'.' -f1 | sed 's/ /T/' )"
  public_url="${R2_PUBLIC_BASE_URL%/}/$key_path"
  artwork_url="/assets/brand/catalog-cover.png"

  node - <<NODE
const fs = require('fs');
const crypto = require('crypto');
const path = '$tmp_json';
const arr = JSON.parse(fs.readFileSync(path, 'utf8'));
const name = ${name@Q};
arr.push({
  key: crypto.createHash('sha1').update(name).digest('hex').slice(0, 16),
  file_name: name,
  title: name.replace(/\.[^/.]+$/, ''),
  size: Number(${size}),
  uploaded_at: new Date('${mtime}Z').toISOString(),
  public_url: ${public_url@Q},
  full_song_url: ${public_url@Q},
  artwork_url: ${artwork_url@Q},
  full_song: true
});
fs.writeFileSync(path, JSON.stringify(arr, null, 2));
NODE

done < <(find "$SOURCE_DIR" -maxdepth 1 -type f \( -iname '*.wav' -o -iname '*.mp3' -o -iname '*.flac' -o -iname '*.m4a' -o -iname '*.aiff' -o -iname '*.ogg' \) -print0 | sort -z)

cp "$tmp_json" "$ROOT_DIR/server/storage/catalog.json"
rm -f "$tmp_json"

echo "Cloud catalog publish complete. Updated server/storage/catalog.json"
