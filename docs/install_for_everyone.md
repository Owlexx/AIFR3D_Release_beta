# Install For Everyone (Step-by-Step)

This page is written for non-developers.

## 1) Pick your platform package

Go to GitHub repo -> `Actions` -> `Ecosystem Release` -> `Run workflow`.

Download `ecosystem-installer-bundle`, extract it, then choose:

- Garuda/Arch Linux: `installer-arch` (`*-archlinux-x64.tar.gz`)
- Ubuntu/Debian Linux: `installer-linux` (`*-linux-x64.tar.gz`)
- Windows: `installer-windows` (`*-windows-x64-setup.exe` and `*-windows-x64.zip`)
- Android: `installer-android` (`app-debug.apk`)

## 2) Linux install (Garuda/Arch and other Linux)

### If you downloaded a release archive from GitHub

From extracted package folder:

```bash
chmod +x install.sh
./install.sh
```

Or from repo root:

```bash
chmod +x scripts/install_from_archive_linux.sh
./scripts/install_from_archive_linux.sh /path/to/v1.0.1-alpha-archlinux-x64.tar.gz
```

### If you built locally from source

From repo root:

```bash
chmod +x scripts/install_local_linux.sh
./scripts/install_local_linux.sh linux-release Release
```

### Where files are installed

- VST3 plugin folder: `~/.vst3/AIFRED.vst3`
- Standalone executable: `~/.local/bin/aifred-standalone`
- Desktop admin executable: `~/.local/bin/dawai-admin`
- App menu shortcuts:
  - `~/.local/share/applications/aifred-standalone.desktop`
  - `~/.local/share/applications/dawai-admin.desktop`

### Launch commands

```bash
~/.local/bin/aifred-standalone
~/.local/bin/dawai-admin
```

## 3) Windows install

### If you downloaded the one-click setup installer (recommended)

1. Run `vX.Y.Z-windows-x64-setup.exe`.
2. Follow the install wizard.
3. Launch `DawAI Admin` from Start Menu.

### If you downloaded a release zip from GitHub

Open PowerShell in repo root and run:

```powershell
.\scripts\install_from_archive_windows.ps1 -ArchivePath C:\path\to\v1.0.1-alpha-windows-x64.zip
```

### If you are building from source on Windows

Open PowerShell in repo root and run:

```powershell
.\scripts\install_local_windows.ps1 -Preset windows-release -Config Release
```

Installed paths:

- VST3 plugin: `%CommonProgramFiles%\VST3\AIFRED.vst3`
- Standalone + admin executables: `%LOCALAPPDATA%\Programs\DawAI`

## 4) Android install

1. Download `app-debug.apk` from workflow artifacts.
2. Copy APK to your Android phone.
3. Open APK and allow installation from unknown sources when prompted.
4. Launch the app named `DawAI Admin`.

## 5) Load plugin in your DAW

1. Open your DAW plugin manager/settings.
2. Add plugin scan path:
   - Linux: `~/.vst3`
   - Windows: `%CommonProgramFiles%\VST3`
3. Click plugin rescan.
4. Insert `AIFRED` on your master bus.

## 6) If you cannot find installed files

Run these commands on Linux:

```bash
ls -la ~/.vst3/AIFRED.vst3
ls -la ~/.local/bin/aifred-standalone ~/.local/bin/dawai-admin
```

If those files exist, installation succeeded.

## 7) One-source-of-truth files (shared brain)

These are the shared truth files used across plugin/admin/apps:

- `assets/aifr3d_brain/brain_config.json`
- `docs/dsp_math_pack_v1.md`
- `docs/schema/report.schema.json`
