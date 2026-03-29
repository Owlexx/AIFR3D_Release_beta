#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VERSION="${1:-2.2.4}"
VERSION="${VERSION#v}"
VARIANTS=(a b c d)

require_variant_artifacts() {
  local suffix upper
  for suffix in "${VARIANTS[@]}"; do
    upper="${suffix^^}"
    local vst="$ROOT_DIR/build/Source/aifred_vst3_${upper}_artefacts/VST3/AIFRED_${suffix}.vst3"
    local standalone="$ROOT_DIR/build/Source/aifred_vst3_${upper}_artefacts/Standalone/AIFRED_${suffix}"
    if [[ ! -d "$vst" ]]; then
      echo "Missing VST artifact: $vst" >&2
      exit 1
    fi
    if [[ ! -x "$standalone" ]]; then
      echo "Missing standalone artifact: $standalone" >&2
      exit 1
    fi
  done
}

DIST_DIR="$ROOT_DIR/dist"
DEB_ROOT="$DIST_DIR/deb_root"
ARCH_ROOT="$DIST_DIR/arch_root"
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

require_variant_artifacts

rm -rf "$DEB_ROOT" "$ARCH_ROOT"
mkdir -p "$DIST_DIR"

mkdir -p "$DEB_ROOT/DEBIAN" "$DEB_ROOT/usr/local/bin" "$DEB_ROOT/usr/local/lib/vst3" "$DEB_ROOT/usr/local/share/dawai/scripts"
mkdir -p "$DEB_ROOT/usr/local/share/dawai/licensing" "$DEB_ROOT/usr/local/share/dawai/assets"
mkdir -p "$DEB_ROOT/usr/local/share/applications" "$DEB_ROOT/usr/local/share/icons/hicolor/256x256/apps" "$DEB_ROOT/usr/local/share/icons/hicolor/512x512/apps"

for suffix in "${VARIANTS[@]}"; do
  upper="${suffix^^}"
  install -m 0755 "$ROOT_DIR/build/Source/aifred_vst3_${upper}_artefacts/Standalone/AIFRED_${suffix}" \
    "$DEB_ROOT/usr/local/bin/aifred_standalone_v${VERSION}${suffix}"
  cp -a "$ROOT_DIR/build/Source/aifred_vst3_${upper}_artefacts/VST3/AIFRED_${suffix}.vst3" \
    "$DEB_ROOT/usr/local/lib/vst3/AIFRED_v${VERSION}${suffix}.vst3"

done
ln -sfn "/usr/local/bin/aifred_standalone_v${VERSION}a" "$DEB_ROOT/usr/local/bin/aifred_standalone"

install -m 0755 "$ROOT_DIR/scripts/install_local_linux.sh" "$DEB_ROOT/usr/local/share/dawai/scripts/install_local_linux.sh"
install -m 0755 "$ROOT_DIR/scripts/install_vst_linux.sh" "$DEB_ROOT/usr/local/share/dawai/scripts/install_vst_linux.sh"
install -m 0755 "$ROOT_DIR/scripts/install_standalone_linux.sh" "$DEB_ROOT/usr/local/share/dawai/scripts/install_standalone_linux.sh"
install -m 0755 "$ROOT_DIR/scripts/install_arch.sh" "$DEB_ROOT/usr/local/share/dawai/scripts/install_arch.sh"
install -m 0644 "$ROOT_DIR/assets/licensing/payment_unlock_v2_2_4_beta.json" "$DEB_ROOT/usr/local/share/dawai/licensing/payment_unlock_v2_2_4_beta.json"
if [[ -f "$ROOT_DIR/assets/branding/mascot_splash_2_2_4_beta.png" ]]; then
  install -m 0644 "$ROOT_DIR/assets/branding/mascot_splash_2_2_4_beta.png" "$DEB_ROOT/usr/local/share/dawai/assets/mascot_splash_2_2_4_beta.png"
fi
if [[ -n "$ICON_SOURCE" ]]; then
  install -m 0644 "$ICON_SOURCE" "$DEB_ROOT/usr/local/share/icons/hicolor/512x512/apps/dawai.png"
  install -m 0644 "$ICON_SOURCE" "$DEB_ROOT/usr/local/share/icons/hicolor/256x256/apps/dawai.png"
fi

cat > "$DEB_ROOT/usr/local/share/applications/dawai-standalone.desktop" <<DESKTOP
[Desktop Entry]
Version=1.0
Type=Application
Name=AIFR3D Standalone 2.2.4 Beta
Comment=AIFR3D standalone default launcher (variant A) 2.2.4 Beta
Exec=/usr/local/bin/aifred_standalone
Icon=dawai
Terminal=false
Categories=AudioVideo;Audio;
StartupNotify=true
DESKTOP

for suffix in "${VARIANTS[@]}"; do
  cat > "$DEB_ROOT/usr/local/share/applications/dawai-standalone-${suffix}.desktop" <<DESKTOP
[Desktop Entry]
Version=1.0
Type=Application
Name=AIFR3D Standalone ${suffix^^} 2.2.4 Beta
Comment=AIFR3D standalone variant ${suffix^^} 2.2.4 Beta
Exec=/usr/local/bin/aifred_standalone_v${VERSION}${suffix}
Icon=dawai
Terminal=false
Categories=AudioVideo;Audio;
StartupNotify=true
DESKTOP
done

cat > "$DEB_ROOT/DEBIAN/control" <<CTRL
Package: dawai-ecosystem
Version: $VERSION
Section: sound
Priority: optional
Architecture: amd64
Maintainer: North3rnLight3r <north3rnlight3rofficial@outlook.com>
Depends: bash, libc6
Description: DawAI VST3/Standalone variants bundle
 Includes A/B/C/D standalone binaries and VST3 bundles with shared DSP.
CTRL

DEB_FILE="$DIST_DIR/dawai-ecosystem_${VERSION}_amd64.deb"
if command -v dpkg-deb >/dev/null 2>&1; then
  dpkg-deb --build "$DEB_ROOT" "$DEB_FILE"
else
  TMP_DEB="$(mktemp -d)"
  echo "2.0" > "$TMP_DEB/debian-binary"
  tar -C "$DEB_ROOT/DEBIAN" -czf "$TMP_DEB/control.tar.gz" .
  tar -C "$DEB_ROOT" --exclude="./DEBIAN" -czf "$TMP_DEB/data.tar.gz" .
  rm -f "$DEB_FILE"
  ar r "$DEB_FILE" "$TMP_DEB/debian-binary" "$TMP_DEB/control.tar.gz" "$TMP_DEB/data.tar.gz" >/dev/null
  rm -rf "$TMP_DEB"
fi
cp -f "$DEB_FILE" "$DIST_DIR/dawai-linux-amd64.deb"

mkdir -p "$ARCH_ROOT/usr/bin" "$ARCH_ROOT/usr/lib/vst3" "$ARCH_ROOT/usr/share/dawai/scripts"
mkdir -p "$ARCH_ROOT/usr/share/dawai/licensing" "$ARCH_ROOT/usr/share/dawai/assets"
mkdir -p "$ARCH_ROOT/usr/share/applications" "$ARCH_ROOT/usr/share/icons/hicolor/256x256/apps" "$ARCH_ROOT/usr/share/icons/hicolor/512x512/apps"
for suffix in "${VARIANTS[@]}"; do
  upper="${suffix^^}"
  install -m 0755 "$ROOT_DIR/build/Source/aifred_vst3_${upper}_artefacts/Standalone/AIFRED_${suffix}" \
    "$ARCH_ROOT/usr/bin/aifred_standalone_v${VERSION}${suffix}"
  cp -a "$ROOT_DIR/build/Source/aifred_vst3_${upper}_artefacts/VST3/AIFRED_${suffix}.vst3" \
    "$ARCH_ROOT/usr/lib/vst3/AIFRED_v${VERSION}${suffix}.vst3"
done
ln -sfn "/usr/bin/aifred_standalone_v${VERSION}a" "$ARCH_ROOT/usr/bin/aifred_standalone"

install -m 0755 "$ROOT_DIR/scripts/install_local_linux.sh" "$ARCH_ROOT/usr/share/dawai/scripts/install_local_linux.sh"
install -m 0755 "$ROOT_DIR/scripts/install_vst_linux.sh" "$ARCH_ROOT/usr/share/dawai/scripts/install_vst_linux.sh"
install -m 0755 "$ROOT_DIR/scripts/install_standalone_linux.sh" "$ARCH_ROOT/usr/share/dawai/scripts/install_standalone_linux.sh"
install -m 0755 "$ROOT_DIR/scripts/install_arch.sh" "$ARCH_ROOT/usr/share/dawai/scripts/install_arch.sh"
install -m 0644 "$ROOT_DIR/assets/licensing/payment_unlock_v2_2_4_beta.json" "$ARCH_ROOT/usr/share/dawai/licensing/payment_unlock_v2_2_4_beta.json"
if [[ -f "$ROOT_DIR/assets/branding/mascot_splash_2_2_4_beta.png" ]]; then
  install -m 0644 "$ROOT_DIR/assets/branding/mascot_splash_2_2_4_beta.png" "$ARCH_ROOT/usr/share/dawai/assets/mascot_splash_2_2_4_beta.png"
fi
if [[ -n "$ICON_SOURCE" ]]; then
  install -m 0644 "$ICON_SOURCE" "$ARCH_ROOT/usr/share/icons/hicolor/512x512/apps/dawai.png"
  install -m 0644 "$ICON_SOURCE" "$ARCH_ROOT/usr/share/icons/hicolor/256x256/apps/dawai.png"
fi

cat > "$ARCH_ROOT/usr/share/applications/dawai-standalone.desktop" <<DESKTOP
[Desktop Entry]
Version=1.0
Type=Application
Name=AIFR3D Standalone 2.2.4 Beta
Comment=AIFR3D standalone default launcher (variant A) 2.2.4 Beta
Exec=/usr/bin/aifred_standalone
Icon=dawai
Terminal=false
Categories=AudioVideo;Audio;
StartupNotify=true
DESKTOP

for suffix in "${VARIANTS[@]}"; do
  cat > "$ARCH_ROOT/usr/share/applications/dawai-standalone-${suffix}.desktop" <<DESKTOP
[Desktop Entry]
Version=1.0
Type=Application
Name=AIFR3D Standalone ${suffix^^} 2.2.4 Beta
Comment=AIFR3D standalone variant ${suffix^^} 2.2.4 Beta
Exec=/usr/bin/aifred_standalone_v${VERSION}${suffix}
Icon=dawai
Terminal=false
Categories=AudioVideo;Audio;
StartupNotify=true
DESKTOP
done

cat > "$ARCH_ROOT/.PKGINFO" <<PKG
pkgname = dawai-ecosystem
pkgbase = dawai-ecosystem
pkgver = $VERSION-1
pkgdesc = DawAI VST3/Standalone A/B/C/D bundle
url = https://github.com/kaeganscott26/DawAI_Ecosystem
builddate = $(date +%s)
packager = North3rnLight3r <north3rnlight3rofficial@outlook.com>
size = $(du -sb "$ARCH_ROOT" | awk '{print $1}')
arch = x86_64
license = proprietary
depend = bash
PKG

ARCH_FILE="$DIST_DIR/dawai-ecosystem-${VERSION}-1-x86_64.pkg.tar.zst"
tar --zstd -cf "$ARCH_FILE" -C "$ARCH_ROOT" .
cp -f "$ARCH_FILE" "$DIST_DIR/dawai-linux-arch-x86_64.pkg.tar.zst"

echo "Created:"
echo "  $DEB_FILE"
echo "  $ARCH_FILE"
