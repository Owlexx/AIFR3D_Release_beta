#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ANDROID_DIR="$ROOT_DIR/apps/android_admin"
LOCAL_PROPS="$ANDROID_DIR/local.properties"

ANDROID_SDK_ROOT="${ANDROID_SDK_ROOT:-$HOME/Android/Sdk}"
if [[ ! -d "$ANDROID_SDK_ROOT" ]]; then
  echo "Android SDK not found at $ANDROID_SDK_ROOT"
  exit 1
fi

existing_props=""
if [[ -f "$LOCAL_PROPS" ]]; then
  existing_props="$(grep -v '^sdk\.dir=' "$LOCAL_PROPS" || true)"
fi

{
  printf 'sdk.dir=%s\n' "$ANDROID_SDK_ROOT"
  if [[ -n "${DAWAI_ADMIN_USERNAME:-}" ]]; then
    printf 'dawaiAdminUsername=%s\n' "$DAWAI_ADMIN_USERNAME"
  fi
  if [[ -n "${DAWAI_ADMIN_PASSWORD:-}" ]]; then
    printf 'dawaiAdminPassword=%s\n' "$DAWAI_ADMIN_PASSWORD"
  fi
  if [[ -n "$existing_props" ]]; then
    printf '%s\n' "$existing_props"
  fi
} > "$LOCAL_PROPS"

export ANDROID_HOME="$ANDROID_SDK_ROOT"
export ANDROID_SDK_ROOT="$ANDROID_SDK_ROOT"

cd "$ANDROID_DIR"
./gradlew :app:assembleDebug

APK_PATH="$ANDROID_DIR/app/build/outputs/apk/debug/app-debug.apk"
if [[ ! -f "$APK_PATH" ]]; then
  echo "APK not found at $APK_PATH"
  exit 1
fi

adb start-server
adb install -r "$APK_PATH"

echo "Android install complete: $APK_PATH"
