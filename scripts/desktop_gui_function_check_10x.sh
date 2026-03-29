#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LOG_DIR="$ROOT_DIR/logs/validation"
mkdir -p "$LOG_DIR"
LOG_FILE="$LOG_DIR/desktop_gui_function_check_10x_$(date +%Y%m%d_%H%M%S).log"

require_pattern() {
  local pattern="$1"
  local file="$2"
  if ! rg -q "$pattern" "$file"; then
    echo "missing pattern '$pattern' in $file" | tee -a "$LOG_FILE"
    return 1
  fi
}

validate_reference_pool() {
  python3 - << 'PY'
import json,sys,pathlib
root=pathlib.Path('.').resolve()
manifest=root/'assets'/'reference_pools'/'pro_25x6'/'pool_manifest.json'
if not manifest.exists():
    print(f"missing manifest: {manifest}")
    sys.exit(1)
j=json.loads(manifest.read_text())
if j.get('counts',{}).get('total_references') != 25:
    print('expected total_references=25')
    sys.exit(1)
required={'rap','hip-hop','edm','dubstep','pop','rock'}
genres=set(j.get('counts',{}).get('by_genre',{}).keys())
if not required.issubset(genres):
    print(f'missing genres: {sorted(required-genres)}')
    sys.exit(1)
print('reference pool ok')
PY
}

{
  echo "desktop-gui-function-check-10x"
  echo "started: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "root: $ROOT_DIR"
} > "$LOG_FILE"

for pass in $(seq 1 10); do
  {
    echo "===== PASS $pass/10 ====="
    date -u +"%Y-%m-%dT%H:%M:%SZ"

    validate_reference_pool

    cmake --build --preset linux-debug -j8
    ctest --preset linux-debug --output-on-failure

    require_pattern "m_applyButton.onClick" "$ROOT_DIR/modules/ui/src/main_view.cpp"
    require_pattern "m_previewButton.onClick" "$ROOT_DIR/modules/ui/src/main_view.cpp"
    require_pattern "m_undoButton.onClick" "$ROOT_DIR/modules/ui/src/main_view.cpp"
    require_pattern "m_candles.setCandles" "$ROOT_DIR/modules/ui/src/main_view.cpp"
    require_pattern "GenreDetector" "$ROOT_DIR/modules/ui/include/dawai/ui/main_view.hpp"
    require_pattern "Dubstep" "$ROOT_DIR/Source/DSP/ReferenceModel.cpp"
    require_pattern "Rock" "$ROOT_DIR/Source/DSP/ReferenceModel.cpp"
    require_pattern "firstSessionMixAverage" "$ROOT_DIR/Source/UI/UiModel.cpp"

    echo "pass $pass ok"
    echo
  } >> "$LOG_FILE" 2>&1

done

echo "desktop gui function check complete: $LOG_FILE"
