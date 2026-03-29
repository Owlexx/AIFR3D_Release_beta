#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LOG_DIR="$ROOT_DIR/logs/validation"
mkdir -p "$LOG_DIR"
LOG_FILE="$LOG_DIR/exhaustive_sweep_$(date +%Y%m%d_%H%M%S).log"
PASSES="${1:-3}"

run_step() {
  local label="$1"
  shift
  echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] STEP: $label" | tee -a "$LOG_FILE"
  "$@" >> "$LOG_FILE" 2>&1
}

for pass in $(seq 1 "$PASSES"); do
  echo "===== EXHAUSTIVE PASS $pass/$PASSES =====" | tee -a "$LOG_FILE"

  run_step "node-check" node --check "$ROOT_DIR/server/src/index.js"
  run_step "website-js-check" node --check "$ROOT_DIR/apps/website/app.js"
  run_step "cmake-build" cmake --build "$ROOT_DIR/build" --parallel
  run_step "standalone-smoke" "$HOME/.local/bin/dawai_standalone"
  run_step "vst-smoke" "$HOME/.local/bin/dawai_vst3_stub"
  run_step "validate-25" "$ROOT_DIR/scripts/validate_25_passes.sh"
  run_step "website-ux-qa" node "$ROOT_DIR/scripts/ux_qa_website.js"

  echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] PASS $pass complete" | tee -a "$LOG_FILE"
  echo | tee -a "$LOG_FILE"
done

echo "Exhaustive sweep complete: $LOG_FILE"
