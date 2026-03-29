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
DEST_DIR="${DEST_DIR:-$ROOT_DIR/apps/website/assets/audio/catalog}"
MODE="${MODE:-hardlink}"

log() {
  printf '[catalog-stage] %s\n' "$1"
}

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

mkdir -p "$DEST_DIR"
find "$DEST_DIR" -maxdepth 1 -type f ! -name '.gitkeep' -delete

while IFS= read -r -d '' file; do
  name="$(basename "$file")"
  target="$DEST_DIR/$name"
  if [[ "$MODE" == "hardlink" ]]; then
    ln -f "$file" "$target" 2>/dev/null || cp -f "$file" "$target"
  else
    cp -f "$file" "$target"
  fi
  log "staged $name"
done < <(find "$SOURCE_DIR" -maxdepth 1 -type f \( -iname '*.wav' -o -iname '*.mp3' -o -iname '*.flac' -o -iname '*.m4a' -o -iname '*.aiff' -o -iname '*.aif' -o -iname '*.ogg' \) -print0 | sort -z)

log "Assets ready in $DEST_DIR"
