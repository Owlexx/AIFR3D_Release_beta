#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LOG_DIR="$ROOT_DIR/logs/validation"
mkdir -p "$LOG_DIR"
LOG_FILE="$LOG_DIR/validate_25_passes_$(date +%Y%m%d_%H%M%S).log"

API_BASE="${API_BASE:-http://127.0.0.1:8787}"
API_TOKEN="${DAWAI_API_TOKEN:-}"

curl_json() {
  local path="$1"
  if [[ -n "$API_TOKEN" ]]; then
    curl -fsS -H "Authorization: Bearer $API_TOKEN" "$API_BASE$path"
  else
    curl -fsS "$API_BASE$path"
  fi
}

curl_post_json() {
  local path="$1"
  local body="$2"
  if [[ -n "$API_TOKEN" ]]; then
    curl -fsS -H "Authorization: Bearer $API_TOKEN" -H "Content-Type: application/json" -d "$body" "$API_BASE$path"
  else
    curl -fsS -H "Content-Type: application/json" -d "$body" "$API_BASE$path"
  fi
}

pass=1
while [[ $pass -le 25 ]]; do
  {
    echo "===== PASS $pass ====="
    date -u +"%Y-%m-%dT%H:%M:%SZ"

    curl_json "/api/v1/health"
    curl_json "/api/v1/catalog/list"

    curl_post_json "/api/v1/command/run" '{"command_line":"echo pass-check"}'

    curl_post_json "/api/v1/memory/feedback" '{"session_id":"validate-loop","action_id":"analysis.run","accepted":true}'

    echo
  } >> "$LOG_FILE" 2>&1

  pass=$((pass + 1))
done

echo "Validation complete: $LOG_FILE"
