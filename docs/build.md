# Build Instructions

## Prerequisites

- CMake 3.24+
- Ninja
- C++20 compiler
- Git (for CPM dependencies)

## Configure and Build

Linux:

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug
```

Windows:

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug
```

Windows Release:

```powershell
cmake --preset windows-release
cmake --build --preset windows-release
```

Platform note:

- Build Windows binaries on Windows (native toolchain).
- Build Linux/Arch binaries on Linux (or Arch/Garuda).
- Do not expect Linux hosts to produce native Windows installers without a dedicated cross-toolchain setup.

## Test

```bash
ctest --preset linux-debug --output-on-failure
```

## Android Admin App

```bash
./scripts/sync_android_truth_assets.sh
cd apps/dawai_admin_android
echo "sdk.dir=$HOME/Android/Sdk" > local.properties
$HOME/Android/Sdk/cmdline-tools/latest/bin/sdkmanager "platforms;android-35" "build-tools;35.0.0"
./gradlew assembleDebug
```

Debug APK output:

- `apps/dawai_admin_android/app/build/outputs/apk/debug/app-debug.apk`

## GitHub Actions Installers

Manual workflow: `.github/workflows/installers.yml`

From GitHub:

1. `Actions` -> `Build Installers` -> `Run workflow`
2. Choose version + target platforms
3. Download generated artifacts (`.tar.gz`, `.zip`, `.apk`) from the run summary

## Local Release Packaging

Linux:

```bash
./scripts/build_release.sh 1.0.0-alpha
```

Outputs:

- Folder: `releases/1.0.0-alpha/`
- Archive: `releases/1.0.0-alpha-linux-x64.tar.gz`

Windows:

```powershell
.\scripts\build_release.ps1 -Version 1.0.0-alpha
```

Outputs:

- Folder: `releases/1.0.0-alpha/`
- Archive: `releases/1.0.0-alpha-windows-x64.zip`
