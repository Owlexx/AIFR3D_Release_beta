# AIFR3D 2.2.4 Beta Windows Build Guide

## Rule

Windows binaries are built on Windows from source. This repo does not publish packaged Windows installers.

## Requirements

- Windows 10/11 x64
- Visual Studio 2022 with Desktop development with C++
- CMake 3.24+
- Git

## Build

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

## Helper Script

```powershell
./scripts/install_windows.ps1 -Target all
```

## Outputs

- VST3 bundle in the Windows build artifact tree
- standalone executable in the Windows build artifact tree
