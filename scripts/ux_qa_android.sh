#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="${1:-$ROOT_DIR/logs/qa/android_$(date +%Y%m%d_%H%M%S)}"
PKG="com.dawai.admin"
ACTIVITY="com.dawai.admin.MainActivity"

mkdir -p "$OUT_DIR"
DUMP_XML="$OUT_DIR/window_dump.xml"
REPORT="$OUT_DIR/summary.txt"

pass_count=0
fail_count=0

record() {
  local ok="$1"
  local step="$2"
  local detail="${3:-}"
  if [[ "$ok" == "1" ]]; then
    pass_count=$((pass_count + 1))
    printf '[PASS] %s %s\n' "$step" "$detail" | tee -a "$REPORT"
  else
    fail_count=$((fail_count + 1))
    printf '[FAIL] %s %s\n' "$step" "$detail" | tee -a "$REPORT"
  fi
}

dump_ui() {
  adb shell uiautomator dump /sdcard/window_dump.xml >/dev/null 2>&1
  adb exec-out cat /sdcard/window_dump.xml > "$DUMP_XML"
}

screenshot() {
  local name="$1"
  adb exec-out screencap -p > "$OUT_DIR/${name}.png"
}

has_text() {
  local text="$1"
  python3 - "$DUMP_XML" "$text" <<'PY'
import sys
import xml.etree.ElementTree as ET
xml_path, needle = sys.argv[1], sys.argv[2]
needle = needle.lower()
try:
    tree = ET.parse(xml_path)
except Exception:
    print('0')
    raise SystemExit(0)
root = tree.getroot()
for node in root.iter('node'):
    txt = (node.attrib.get('text') or '')
    desc = (node.attrib.get('content-desc') or '')
    if needle in txt.lower() or needle in desc.lower():
        print('1')
        raise SystemExit(0)
print('0')
PY
}

tap_text() {
  local text="$1"
  python3 - "$DUMP_XML" "$text" <<'PY'
import re
import sys
import xml.etree.ElementTree as ET
xml_path, needle = sys.argv[1], sys.argv[2]
needle = needle.lower()

try:
    root = ET.parse(xml_path).getroot()
except Exception:
    print('ERR')
    raise SystemExit(0)

def emit_bounds(node):
    bounds = node.attrib.get('bounds', '')
    m = re.match(r'\[(\d+),(\d+)\]\[(\d+),(\d+)\]', bounds)
    if not m:
        return False
    x1, y1, x2, y2 = map(int, m.groups())
    cx = (x1 + x2) // 2
    cy = (y1 + y2) // 2
    print(f"{cx} {cy}")
    return True

# Prefer exact text/desc match first to avoid false taps
# like matching "DawAI Admin Command Center" when searching "Command".
for node in root.iter('node'):
    txt = (node.attrib.get('text') or '').strip().lower()
    desc = (node.attrib.get('content-desc') or '').strip().lower()
    if txt == needle or desc == needle:
        if emit_bounds(node):
            raise SystemExit(0)

for node in root.iter('node'):
    txt = (node.attrib.get('text') or '').strip().lower()
    desc = (node.attrib.get('content-desc') or '').strip().lower()
    if needle in txt or needle in desc:
        if emit_bounds(node):
            raise SystemExit(0)

print('ERR')
PY
}

perform_tap_by_text() {
  local label="$1"
  dump_ui
  local coords
  coords="$(tap_text "$label")"
  if [[ "$coords" == "ERR" ]]; then
    record 0 "tap-$label" "not found"
    return 1
  fi
  adb shell input tap ${coords} >/dev/null 2>&1 || true
  sleep 1
  record 1 "tap-$label"
  return 0
}

adb get-state >/dev/null 2>&1 || {
  echo "No adb device connected" >&2
  exit 1
}

: > "$REPORT"

adb shell am start -n "$PKG/$ACTIVITY" >/dev/null 2>&1 || true
sleep 2

dump_ui
screenshot "00_launch"

if [[ "$(has_text 'DawAI Admin Command Center')" == "1" ]]; then
  record 1 "launch-title"
else
  record 0 "launch-title"
fi

for text in "Chat" "Upload" "Command"; do
  if [[ "$(has_text "$text")" == "1" ]]; then
    record 1 "has-tab-$text"
  else
    record 0 "has-tab-$text"
  fi
done

perform_tap_by_text "Upload" || true
dump_ui
screenshot "01_upload_tab"
for text in "Admin Username" "Admin Password" "Login" "Open Web Admin" "Select File" "Upload"; do
  if [[ "$(has_text "$text")" == "1" ]]; then
    record 1 "upload-has-$text"
  else
    record 0 "upload-has-$text"
  fi
done

perform_tap_by_text "Open Web Admin" || true
sleep 2
screenshot "02_after_open_web_admin"
adb shell input keyevent KEYCODE_BACK >/dev/null 2>&1 || true
sleep 1
adb shell am start -n "$PKG/$ACTIVITY" >/dev/null 2>&1 || true
sleep 1
perform_tap_by_text "Upload" || true

perform_tap_by_text "Select File" || true
sleep 1
screenshot "03_after_select_file"
adb shell input keyevent KEYCODE_BACK >/dev/null 2>&1 || true
sleep 1

perform_tap_by_text "Login" || true
dump_ui
if [[ "$(has_text 'admin login required')" == "1" ]]; then
  record 1 "upload-login-validation"
else
  record 0 "upload-login-validation"
fi

perform_tap_by_text "Command" || true
dump_ui
screenshot "04_command_tab"
for text in "Run" "curl -s https://www.north3rnlight3r.com/api/v1/health" "./build/dawai_standalone" "./build/dawai_vst3_stub"; do
  if [[ "$(has_text "$text")" == "1" ]]; then
    record 1 "command-has-$text"
  else
    record 0 "command-has-$text"
  fi
done

perform_tap_by_text "Run" || true
sleep 2
screenshot "05_command_run"

perform_tap_by_text "Chat" || true
dump_ui
screenshot "06_chat_tab"
for text in "Prompt" "Send"; do
  if [[ "$(has_text "$text")" == "1" ]]; then
    record 1 "chat-has-$text"
  else
    record 0 "chat-has-$text"
  fi
done
perform_tap_by_text "Send" || true
sleep 1
screenshot "07_chat_send"

printf '\nSummary: pass=%d fail=%d\n' "$pass_count" "$fail_count" | tee -a "$REPORT"
if [[ "$fail_count" -gt 0 ]]; then
  exit 1
fi
