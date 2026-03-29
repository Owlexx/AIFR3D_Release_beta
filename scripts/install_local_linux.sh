#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
AUTO_APT="${DAWAI_AUTO_APT:-1}"
TARGET="${DAWAI_INSTALL_TARGET:-all}"
INSTALL_DIR="$HOME/.local/share/dawai/bin"
ASSET_DIR="$HOME/.local/share/dawai/assets"
BIN_DIR="$HOME/.local/bin"
VST_DIR="$HOME/.vst3"
ICON_NAME="dawai"
ICON_INSTALL_DIR_256="$HOME/.local/share/icons/hicolor/256x256/apps"
ICON_INSTALL_DIR_512="$HOME/.local/share/icons/hicolor/512x512/apps"
LAUNCHER_DIR="$HOME/.local/share/applications"
CONFIG_ROOT="${XDG_CONFIG_HOME:-$HOME/.config}"
LICENSE_DIR="$CONFIG_ROOT/dawai/license"
PAYMENT_UNLOCK_SOURCE="$ROOT_DIR/assets/licensing/payment_unlock_v2_2_4_beta.json"
VERSION_TAG="2.2.4"
VARIANT_SUFFIXES=(a b c d)

log() {
  printf '[install] %s\n' "$1"
}

have() {
  command -v "$1" >/dev/null 2>&1
}

resolve_icon_source() {
  local candidates=(
    "$ROOT_DIR/assets/icons/aifred_logo.png"
    "$ROOT_DIR/apps/android_admin/app/src/main/res/drawable/mascot_logo.png"
    "$ROOT_DIR/apps/website/assets/brand/mascot_app_icon.png"
    "$ROOT_DIR/assets/icons/aifred_logo.jpg"
    "$ROOT_DIR/apps/website/assets/brand/mascot_logo.jpg"
  )
  local candidate
  for candidate in "${candidates[@]}"; do
    if [[ -f "$candidate" ]]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done
  return 1
}

require_target() {
  case "$TARGET" in
    all|vst|standalone)
      return 0
      ;;
    *)
      log "Unsupported DAWAI_INSTALL_TARGET=$TARGET (use: all|vst|standalone)"
      exit 1
      ;;
  esac
}

install_with_apt_if_needed() {
  local missing=()

  have curl || missing+=(curl)
  have node || missing+=(nodejs)
  have npm || missing+=(npm)
  have cmake || missing+=(cmake)
  have g++ || missing+=(g++)
  have make || missing+=(make)
  have python3 || missing+=(python3)

  local build_deps=(
    pkg-config
    libasound2-dev
    libx11-dev
    libxrandr-dev
    libxinerama-dev
    libxcursor-dev
    libxcomposite-dev
    libxext-dev
    libfreetype6-dev
    libgtk-3-dev
  )

  if have apt-get; then
    for dep in "${build_deps[@]}"; do
      dpkg -s "$dep" >/dev/null 2>&1 || missing+=("$dep")
    done
  fi

  if [[ ${#missing[@]} -eq 0 ]]; then
    return 0
  fi

  if [[ "$AUTO_APT" != "1" ]]; then
    log "Missing dependencies: ${missing[*]}"
    log "Set DAWAI_AUTO_APT=1 to auto-install with apt."
    return 1
  fi

  if ! have apt-get; then
    log "Missing dependencies and apt-get not found: ${missing[*]}"
    return 1
  fi

  if [[ "${EUID}" -ne 0 ]]; then
    if have sudo; then
      log "Installing OS dependencies with sudo apt-get"
      sudo apt-get update
      sudo apt-get install -y "${missing[@]}"
    else
      log "Missing dependencies and sudo unavailable: ${missing[*]}"
      return 1
    fi
  else
    log "Installing OS dependencies with apt-get"
    apt-get update
    apt-get install -y "${missing[@]}"
  fi
}

health_check() {
  local server_pid=""

  cd "$ROOT_DIR"
  PORT=8787 node server/src/index.js >/tmp/dawai_install_server.log 2>&1 &
  server_pid="$!"

  cleanup() {
    if [[ -n "$server_pid" ]] && kill -0 "$server_pid" >/dev/null 2>&1; then
      kill "$server_pid" >/dev/null 2>&1 || true
      wait "$server_pid" 2>/dev/null || true
    fi
  }

  trap cleanup EXIT

  for _ in {1..20}; do
    if curl -fsS "http://127.0.0.1:8787/api/v1/health" >/dev/null 2>&1; then
      trap - EXIT
      cleanup
      return 0
    fi
    sleep 1
  done

  log "Server health check failed. See /tmp/dawai_install_server.log"
  return 1
}

build_target() {
  local cmake_targets=(aifred_vst3_A aifred_vst3_B aifred_vst3_C aifred_vst3_D)
  log "Building AIFRED 2.2.4a-d variant targets (${TARGET})"
  cmake --build build --target "${cmake_targets[@]}" --parallel
}

install_payment_unlock_proof() {
  if [[ ! -f "$PAYMENT_UNLOCK_SOURCE" ]]; then
    log "Payment proof bundle missing: $PAYMENT_UNLOCK_SOURCE"
    return 0
  fi
  mkdir -p "$LICENSE_DIR"
  install -m 0644 "$PAYMENT_UNLOCK_SOURCE" "$LICENSE_DIR/payment_unlock_v2_2_4_beta.json"
  log "Installed payment proof bundle: $LICENSE_DIR/payment_unlock_v2_2_4_beta.json"
}

install_runtime_assets() {
  mkdir -p "$ASSET_DIR"
  if [[ -f "$ROOT_DIR/assets/branding/mascot_splash_2_2_4_beta.png" ]]; then
    install -m 0644 "$ROOT_DIR/assets/branding/mascot_splash_2_2_4_beta.png" \
      "$ASSET_DIR/mascot_splash_2_2_4_beta.png"
    log "Installed splash asset: $ASSET_DIR/mascot_splash_2_2_4_beta.png"
  fi
}

install_vst_variants() {
  mkdir -p "$VST_DIR"
  local suffix upper src dst
  for suffix in "${VARIANT_SUFFIXES[@]}"; do
    upper="${suffix^^}"
    src="$ROOT_DIR/build/Source/aifred_vst3_${upper}_artefacts/VST3/AIFRED_${suffix}.vst3"
    dst="$VST_DIR/AIFRED_v${VERSION_TAG}${suffix}.vst3"
    if [[ -d "$src" ]]; then
      rm -rf "$dst"
      cp -a "$src" "$dst"
      log "Installed VST variant ${suffix^^}: $dst"
    else
      log "WARNING: missing VST variant artifact: $src"
    fi
  done
}

install_standalone_variants() {
  mkdir -p "$INSTALL_DIR" "$BIN_DIR"
  local suffix upper src dst
  for suffix in "${VARIANT_SUFFIXES[@]}"; do
    upper="${suffix^^}"
    src="$ROOT_DIR/build/Source/aifred_vst3_${upper}_artefacts/Standalone/AIFRED_${suffix}"
    dst="$INSTALL_DIR/aifred_standalone_v${VERSION_TAG}${suffix}"
    if [[ -x "$src" ]]; then
      install -m 0755 "$src" "$dst"
      ln -sfn "$dst" "$BIN_DIR/aifred_standalone_${suffix}"
      log "Installed standalone variant ${suffix^^}: $dst"
    else
      log "WARNING: missing standalone variant artifact: $src"
    fi
  done
  if [[ -x "$INSTALL_DIR/aifred_standalone_v${VERSION_TAG}a" ]]; then
    ln -sfn "$INSTALL_DIR/aifred_standalone_v${VERSION_TAG}a" "$BIN_DIR/aifred_standalone"
  fi
}

install_target_files() {
  if [[ "$TARGET" == "all" || "$TARGET" == "vst" ]]; then
    install_vst_variants
  fi
  if [[ "$TARGET" == "all" || "$TARGET" == "standalone" ]]; then
    install_standalone_variants
  fi
}

install_launcher_assets() {
  local icon_source=""
  if icon_source="$(resolve_icon_source)"; then
    mkdir -p "$ICON_INSTALL_DIR_256" "$ICON_INSTALL_DIR_512"
    if have convert; then
      convert "$icon_source" -resize 512x512 "$ICON_INSTALL_DIR_512/${ICON_NAME}.png"
      convert "$icon_source" -resize 256x256 "$ICON_INSTALL_DIR_256/${ICON_NAME}.png"
    else
      install -m 0644 "$icon_source" "$ICON_INSTALL_DIR_512/${ICON_NAME}.png"
      install -m 0644 "$icon_source" "$ICON_INSTALL_DIR_256/${ICON_NAME}.png"
    fi
    log "Installed launcher icon: $ICON_INSTALL_DIR_512/${ICON_NAME}.png"
  else
    log "No mascot icon source found; skipping launcher icon install"
  fi

  mkdir -p "$LAUNCHER_DIR"

  if [[ "$TARGET" == "all" || "$TARGET" == "standalone" ]]; then
    local suffix desktop_name exec_path
    for suffix in "${VARIANT_SUFFIXES[@]}"; do
      exec_path="$BIN_DIR/aifred_standalone_${suffix}"
      if [[ -x "$exec_path" ]]; then
        desktop_name="$LAUNCHER_DIR/dawai-standalone-${suffix}.desktop"
        cat > "$desktop_name" <<DESKTOP
[Desktop Entry]
Version=1.0
Type=Application
Name=AIFR3D Standalone ${suffix^^} 2.2.4 Beta
Comment=AIFR3D standalone variant ${suffix^^} 2.2.4 Beta
Exec=env AIFR3D_LOGO_PATH=$ICON_INSTALL_DIR_512/${ICON_NAME}.png $exec_path
Icon=$ICON_NAME
Terminal=false
Categories=AudioVideo;Audio;
StartupNotify=true
DESKTOP
        chmod 0644 "$desktop_name"
        log "Installed launcher entry: $desktop_name"
      fi
    done

    if [[ -x "$BIN_DIR/aifred_standalone" ]]; then
      cat > "$LAUNCHER_DIR/dawai-standalone.desktop" <<DESKTOP
[Desktop Entry]
Version=1.0
Type=Application
Name=AIFR3D Standalone 2.2.4 Beta
Comment=AIFR3D standalone default launcher (variant A) 2.2.4 Beta
Exec=env AIFR3D_LOGO_PATH=$ICON_INSTALL_DIR_512/${ICON_NAME}.png $BIN_DIR/aifred_standalone
Icon=$ICON_NAME
Terminal=false
Categories=AudioVideo;Audio;
StartupNotify=true
DESKTOP
      chmod 0644 "$LAUNCHER_DIR/dawai-standalone.desktop"
      log "Installed launcher entry: $LAUNCHER_DIR/dawai-standalone.desktop"
    fi
  fi

  if have update-desktop-database; then
    update-desktop-database "$LAUNCHER_DIR" >/dev/null 2>&1 || true
  fi
  if have gtk-update-icon-cache; then
    gtk-update-icon-cache -f "$HOME/.local/share/icons/hicolor" >/dev/null 2>&1 || true
  fi
}

require_target
cd "$ROOT_DIR"

log "Install target: $TARGET"
log "Checking and installing dependencies"
install_with_apt_if_needed

log "Installing npm dependencies"
npm install

log "Configuring C++ build"
cmake -S . -B build

build_target

log "Validating backend syntax"
node --check server/src/index.js

log "Running startup health check"
health_check

install_target_files
install_launcher_assets
install_payment_unlock_proof
install_runtime_assets

log "Install complete"
log "Run backend: npm run dev"
if [[ "$TARGET" == "all" || "$TARGET" == "standalone" ]]; then
  log "Run default standalone (A): $BIN_DIR/aifred_standalone"
  log "Run standalone variants: $BIN_DIR/aifred_standalone_{a,b,c,d}"
fi
if [[ "$TARGET" == "all" || "$TARGET" == "vst" ]]; then
  log "Installed VST variants in: $VST_DIR"
fi
