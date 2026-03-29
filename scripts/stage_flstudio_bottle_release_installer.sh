#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RELEASE_TAG="${1:-v2.2.4-beta}"
ASSET_NAME="${2:-plugin-windows-installer.ps1}"
BOTTLE_DRIVE_C="${3:-$HOME/.local/share/bottles/bottles/FLStudio/drive_c}"
STAGE_DIR="$BOTTLE_DRIVE_C/AIFR3D_2_2_4_Beta_INSTALL_HERE"

case "$ASSET_NAME" in
  DawAI-Plugin-Windows-Installer.ps1) ASSET_NAME="plugin-windows-installer.ps1" ;;
  DawAI-Standalone-Windows-Installer.ps1) ASSET_NAME="standalone-windows-installer.ps1" ;;
esac

RELEASE_URL="https://github.com/kaeganscott26/DawAI_Ecosystem/releases/download/${RELEASE_TAG}/${ASSET_NAME}"

mkdir -p "$STAGE_DIR"
find "$STAGE_DIR" -mindepth 1 -maxdepth 1 -type f -delete

cat > "$STAGE_DIR/INSTALL_HERE.ps1" <<EOF
\$ErrorActionPreference = 'Stop'
\$releaseUrl = '${RELEASE_URL}'
\$assetName = '${ASSET_NAME}'
\$downloadPath = Join-Path \$env:TEMP \$assetName

Write-Host "[AIFR3D] Downloading \$assetName from \$releaseUrl"
Invoke-WebRequest -Uri \$releaseUrl -OutFile \$downloadPath

Write-Host "[AIFR3D] Launching \$downloadPath"
if (\$downloadPath.ToLowerInvariant().EndsWith('.ps1')) {
    & powershell -NoProfile -ExecutionPolicy Bypass -File \$downloadPath
    if (\$LASTEXITCODE -ne 0) {
        throw \"Installer script failed with exit code \$LASTEXITCODE\"
    }
} else {
    Start-Process -FilePath \$downloadPath -Wait
}
Write-Host "[AIFR3D] Installer finished."
EOF

cat > "$STAGE_DIR/INSTALL_READY.txt" <<EOF
AIFR3D 2.2.4 Beta bottle handoff

Bundle path:
${STAGE_DIR}

Primary launcher:
INSTALL_HERE.cmd

Fallback launcher:
INSTALL_HERE.ps1

Expected release asset:
${RELEASE_URL}

If the EXE is unavailable, run inside the FLStudio bottle:
powershell -ExecutionPolicy Bypass -File C:\\AIFR3D_2_2_4_Beta_INSTALL_HERE\\INSTALL_HERE.ps1
EOF

cat > "$STAGE_DIR/INSTALL_HERE.cmd" <<'EOF'
@echo off
powershell -ExecutionPolicy Bypass -File C:\AIFR3D_2_2_4_Beta_INSTALL_HERE\INSTALL_HERE.ps1
EOF

cat > "$STAGE_DIR/install_here_launcher.c" <<'EOF'
#include <windows.h>

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR commandLine, int showCommand)
{
    (void)instance;
    (void)previous;
    (void)commandLine;

    const char* command =
        "C:\\windows\\system32\\WindowsPowerShell\\v1.0\\powershell.exe "
        "-ExecutionPolicy Bypass -File C:\\AIFR3D_2_2_4_Beta_INSTALL_HERE\\INSTALL_HERE.ps1";

    STARTUPINFOA startupInfo;
    PROCESS_INFORMATION processInfo;
    ZeroMemory(&startupInfo, sizeof(startupInfo));
    ZeroMemory(&processInfo, sizeof(processInfo));
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.dwFlags = STARTF_USESHOWWINDOW;
    startupInfo.wShowWindow = (WORD)showCommand;

    if (!CreateProcessA(NULL, (LPSTR)command, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo,
                        &processInfo))
    {
        MessageBoxA(NULL, "Failed to start INSTALL_HERE.ps1", "AIFR3D 2.2.4 Beta",
                    MB_ICONERROR | MB_OK);
        return 1;
    }

    WaitForSingleObject(processInfo.hProcess, INFINITE);

    DWORD exitCode = 1;
    GetExitCodeProcess(processInfo.hProcess, &exitCode);
    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);
    return (int)exitCode;
}
EOF

rm -f "$STAGE_DIR/INSTALL_HERE.exe" "$STAGE_DIR/INSTALL_HERE.exe.so"

if command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1; then
    x86_64-w64-mingw32-gcc -mwindows -o "$STAGE_DIR/INSTALL_HERE.exe" \
        "$STAGE_DIR/install_here_launcher.c"
fi

chmod 0644 "$STAGE_DIR/INSTALL_HERE.ps1" "$STAGE_DIR/INSTALL_READY.txt" \
    "$STAGE_DIR/install_here_launcher.c" "$STAGE_DIR/INSTALL_HERE.cmd"

printf '[stage] Bottles handoff staged at %s\n' "$STAGE_DIR"
