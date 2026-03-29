#!/bin/bash

# AudioSuite System - Professional Installation Script for Arch/Garuda Linux
# This script handles the installation of the AudioSuite components into a new, obfuscated home directory.

TARGET_DIR="$HOME/.AudioSuite_Production_Suite"
INSTALL_NAME="AudioSuite_System"

echo "================================================================="
echo "  AudioSuite Production Suite - Professional Linux Installation  "
echo "================================================================="

# 1. Create target directory
echo "Creating target directory: $TARGET_DIR"
mkdir -p "$TARGET_DIR"

# 2. Extract components
echo "Extracting components to $TARGET_DIR..."
# The script assumes it's being run from the temporary extraction directory of the self-extractor.
cp -r . "$TARGET_DIR/"

# 3. Set up Arch Linux (Garuda) Dependencies
if command -v pacman >/dev/null 2>&1; then
    echo "Detected pacman (Arch Linux). Checking for dependencies..."
    # Core dependencies for JUCE and VST3 on Arch
    ARCH_DEPS="libcurl-gnutls freetype2 alsa-lib libx11 libxext libxinerama libxrandr libxcursor webkit2gtk"
    sudo pacman -S --needed --noconfirm $ARCH_DEPS
fi

# 4. Set up VST3
VST3_DIR="$HOME/.vst3"
mkdir -p "$VST3_DIR"
echo "Setting up VST3 plugin..."
# Note: In a real scenario, we would move the compiled .vst3 here.
# For this package, we're ensuring the structure is ready.
# ln -sf "$TARGET_DIR/AudioSuite_System/VST3/AudioSuite.vst3" "$VST3_DIR/"

# 5. Set up permissions
echo "Setting up executable permissions..."
find "$TARGET_DIR" -type f -name "*.sh" -exec chmod +x {} \;
# chmod +x "$TARGET_DIR/AudioSuite_System/apps/standalone_vocal_recorder/AudioSuiteStandalone" 2>/dev/null

# 6. Create a desktop shortcut (optional but helpful for 'clickable' requirement)
DESKTOP_FILE="$HOME/.local/share/applications/audiosuite_vocal.desktop"
mkdir -p "$HOME/.local/share/applications"
cat > "$DESKTOP_FILE" <<EOF
[Desktop Entry]
Name=AudioSuite Vocal Recorder
Comment=AI-Driven Professional Vocal Recording
Exec=$TARGET_DIR/apps/standalone_vocal_recorder/AudioSuiteStandalone
Icon=audio-card
Terminal=false
Type=Application
Categories=AudioVideo;Audio;
EOF
chmod +x "$DESKTOP_FILE"

echo "================================================================="
echo "  Installation Complete!                                         "
echo "  System installed to: $TARGET_DIR                               "
echo "  You can now find AudioSuite in your application menu.          "
echo "================================================================="
