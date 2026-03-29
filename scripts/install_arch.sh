#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VARIANTS=(a b c d)
VERSION="2.2.4"

have() {
  command -v "$1" >/dev/null 2>&1
}

if ! have pacman; then
  echo "pacman is required for Arch install" >&2
  exit 1
fi

NEEDED=()
for cmd in node npm cmake g++ make python3 curl; do
  if ! have "$cmd"; then
    NEEDED+=("$cmd")
  fi
done

if [[ ${#NEEDED[@]} -gt 0 ]]; then
  echo "Installing dependencies: ${NEEDED[*]}"
  sudo pacman -Sy --needed --noconfirm base-devel nodejs npm cmake gcc make python curl
fi

cd "$ROOT_DIR"

echo "Installing npm dependencies"
npm install

echo "Configuring and building variant targets"
cmake -S . -B build
cmake --build build --target aifred_vst3_A aifred_vst3_B aifred_vst3_C aifred_vst3_D --parallel

mkdir -p "$HOME/.local/share/dawai/bin" "$HOME/.local/bin" "$HOME/.vst3"
mkdir -p "$HOME/.local/share/dawai/assets"
for suffix in "${VARIANTS[@]}"; do
  upper="${suffix^^}"
  standalone_src="build/Source/aifred_vst3_${upper}_artefacts/Standalone/AIFRED_${suffix}"
  vst_src="build/Source/aifred_vst3_${upper}_artefacts/VST3/AIFRED_${suffix}.vst3"

  if [[ -x "$standalone_src" ]]; then
    install -m 0755 "$standalone_src" "$HOME/.local/share/dawai/bin/aifred_standalone_v${VERSION}${suffix}"
    ln -sfn "$HOME/.local/share/dawai/bin/aifred_standalone_v${VERSION}${suffix}" "$HOME/.local/bin/aifred_standalone_${suffix}"
  fi

  if [[ -d "$vst_src" ]]; then
    rm -rf "$HOME/.vst3/AIFRED_v${VERSION}${suffix}.vst3"
    cp -a "$vst_src" "$HOME/.vst3/AIFRED_v${VERSION}${suffix}.vst3"
  fi
done

ln -sfn "$HOME/.local/share/dawai/bin/aifred_standalone_v${VERSION}a" "$HOME/.local/bin/aifred_standalone"

if [[ -f "$ROOT_DIR/assets/branding/mascot_splash_2_2_4_beta.png" ]]; then
  install -m 0644 "$ROOT_DIR/assets/branding/mascot_splash_2_2_4_beta.png" \
    "$HOME/.local/share/dawai/assets/mascot_splash_2_2_4_beta.png"
fi

ICON_SOURCE=""
for candidate in \
  "$ROOT_DIR/assets/icons/aifred_logo.png" \
  "$ROOT_DIR/apps/android_admin/app/src/main/res/drawable/mascot_logo.png" \
  "$ROOT_DIR/apps/website/assets/brand/mascot_app_icon.png" \
  "$ROOT_DIR/assets/icons/aifred_logo.jpg" \
  "$ROOT_DIR/apps/website/assets/brand/mascot_logo.jpg"; do
  if [[ -f "$candidate" ]]; then
    ICON_SOURCE="$candidate"
    break
  fi
done

if [[ -n "$ICON_SOURCE" ]]; then
  mkdir -p "$HOME/.local/share/icons/hicolor/256x256/apps" "$HOME/.local/share/icons/hicolor/512x512/apps"
  if have convert; then
    convert "$ICON_SOURCE" -resize 512x512 "$HOME/.local/share/icons/hicolor/512x512/apps/dawai.png"
    convert "$ICON_SOURCE" -resize 256x256 "$HOME/.local/share/icons/hicolor/256x256/apps/dawai.png"
  else
    install -m 0644 "$ICON_SOURCE" "$HOME/.local/share/icons/hicolor/512x512/apps/dawai.png"
    install -m 0644 "$ICON_SOURCE" "$HOME/.local/share/icons/hicolor/256x256/apps/dawai.png"
  fi
fi

mkdir -p "$HOME/.local/share/applications"
cat > "$HOME/.local/share/applications/dawai-standalone.desktop" <<DESKTOP
[Desktop Entry]
Version=1.0
Type=Application
Name=AIFR3D Standalone 2.2.4 Beta
Comment=AIFR3D standalone default launcher (variant A) 2.2.4 Beta
Exec=env AIFR3D_LOGO_PATH=$HOME/.local/share/icons/hicolor/512x512/apps/dawai.png $HOME/.local/bin/aifred_standalone
Icon=dawai
Terminal=false
Categories=AudioVideo;Audio;
StartupNotify=true
DESKTOP

for suffix in "${VARIANTS[@]}"; do
  cat > "$HOME/.local/share/applications/dawai-standalone-${suffix}.desktop" <<DESKTOP
[Desktop Entry]
Version=1.0
Type=Application
Name=AIFR3D Standalone ${suffix^^} 2.2.4 Beta
Comment=AIFR3D standalone variant ${suffix^^} 2.2.4 Beta
Exec=env AIFR3D_LOGO_PATH=$HOME/.local/share/icons/hicolor/512x512/apps/dawai.png $HOME/.local/bin/aifred_standalone_${suffix}
Icon=dawai
Terminal=false
Categories=AudioVideo;Audio;
StartupNotify=true
DESKTOP
done

if have update-desktop-database; then
  update-desktop-database "$HOME/.local/share/applications" >/dev/null 2>&1 || true
fi
if have gtk-update-icon-cache; then
  gtk-update-icon-cache -f "$HOME/.local/share/icons/hicolor" >/dev/null 2>&1 || true
fi

echo "Install complete"
echo "Run default standalone: $HOME/.local/bin/aifred_standalone"
echo "VST variants installed in: $HOME/.vst3"
