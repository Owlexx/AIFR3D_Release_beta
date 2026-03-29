#!/bin/bash

# AIFR3D Admin App - APK Build Script
# Builds, signs, and packages the complete admin app with all features

set -e

echo "╔════════════════════════════════════════════════════════════╗"
echo "║        AIFR3D Admin App - APK Build Script                 ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Configuration
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_TYPE="${1:-release}"
OUTPUT_DIR="$HOME/aifr3d-builds"
KEYSTORE_PATH="$HOME/.android/aifr3d.keystore"
KEYSTORE_ALIAS="aifr3d-key"
KEYSTORE_PASSWORD="${KEYSTORE_PASSWORD:-aifr3d2024}"

echo "📁 Project Directory: $PROJECT_DIR"
echo "🔨 Build Type: $BUILD_TYPE"
echo "📦 Output Directory: $OUTPUT_DIR"
echo ""

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Check if keystore exists, create if not
if [ ! -f "$KEYSTORE_PATH" ]; then
    echo "🔑 Creating keystore..."
    keytool -genkey -v -keystore "$KEYSTORE_PATH" \
        -keyalg RSA -keysize 2048 -validity 10000 \
        -alias "$KEYSTORE_ALIAS" \
        -storepass "$KEYSTORE_PASSWORD" \
        -keypass "$KEYSTORE_PASSWORD" \
        -dname "CN=North3rnLight3r, OU=AIFR3D, O=North3rnLight3r, L=Earth, S=Earth, C=US"
    echo "✅ Keystore created"
else
    echo "✅ Keystore found"
fi

echo ""
echo "🔨 Building APK..."

# Navigate to project directory
cd "$PROJECT_DIR"

# Build the APK
if [ "$BUILD_TYPE" = "release" ]; then
    echo "📦 Building release APK..."
    ./gradlew clean bundleRelease \
        -PCORESYNTH_BASE_URL="https://north3rnlight3r.com" \
        -PCORESYNTH_API_TOKEN="aifr3d-admin-token-2024" \
        -PCORESYNTH_ADMIN_USERNAME="North3rnLight3r" \
        -PCORESYNTH_ADMIN_PASSWORD="Poohbe@r2009\$0826"
    
    # Sign the APK
    echo "🔐 Signing APK..."
    jarsigner -verbose -sigalg SHA1withRSA -digestalg SHA1 \
        -keystore "$KEYSTORE_PATH" \
        -storepass "$KEYSTORE_PASSWORD" \
        -keypass "$KEYSTORE_PASSWORD" \
        app/build/outputs/bundle/release/app-release.aab \
        "$KEYSTORE_ALIAS"
else
    echo "📦 Building debug APK..."
    ./gradlew clean assembleDebug \
        -PCORESYNTH_BASE_URL="https://north3rnlight3r.com" \
        -PCORESYNTH_API_TOKEN="aifr3d-admin-token-2024"
fi

echo ""
echo "📋 Build Output:"

# Find and copy APK files
if [ "$BUILD_TYPE" = "release" ]; then
    APK_SOURCE="app/build/outputs/bundle/release/app-release.aab"
    APK_NAME="aifr3d-admin-v2.2.5-release.aab"
else
    APK_SOURCE="app/build/outputs/apk/debug/app-debug.apk"
    APK_NAME="aifr3d-admin-v2.2.5-debug.apk"
fi

if [ -f "$APK_SOURCE" ]; then
    cp "$APK_SOURCE" "$OUTPUT_DIR/$APK_NAME"
    echo "✅ APK copied to: $OUTPUT_DIR/$APK_NAME"
    echo "   Size: $(du -h "$OUTPUT_DIR/$APK_NAME" | cut -f1)"
else
    echo "❌ APK not found at: $APK_SOURCE"
    exit 1
fi

echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║              Build Complete!                               ║"
echo "║                                                            ║"
echo "║  APK Location: $OUTPUT_DIR/$APK_NAME"
echo "║  Version: 2.2.5                                            ║"
echo "║  Build Type: $BUILD_TYPE                                   ║"
echo "║                                                            ║"
echo "║  Features:                                                 ║"
echo "║  ✓ WebSocket Terminal (Local Bridge)                       ║"
echo "║  ✓ R2 Storage Integration (Beat Catalog)                   ║"
echo "║  ✓ Google Drive MCP                                        ║"
echo "║  ✓ Cloudflare AI Gateway Routing                           ║"
echo "║  ✓ Command Execution (with fakeroot/tsu)                   ║"
echo "║  ✓ Local LLM Support (Ollama/OpenClaw)                     ║"
echo "║                                                            ║"
echo "║  Installation:                                             ║"
echo "║  adb install $OUTPUT_DIR/$APK_NAME                         ║"
echo "║                                                            ║"
echo "╚════════════════════════════════════════════════════════════╝"
