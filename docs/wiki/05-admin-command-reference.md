# Admin and Console Command Reference

This page lists the command surface across the DawAI ecosystem.

## A) Implemented `DawAI` CLI flags (today)

Binary: `DawAI` (desktop app executable with CLI mode)

Source of truth: `apps/dawai/src/main.cpp`.

### Help

```bash
DawAI --help
DawAI -h
```

### Build reference cache from pool JSON

```bash
DawAI --build-reference-cache /path/to/pool.json
```

### Build reference pool from audio directory

```bash
DawAI --build-reference-pool /path/to/audio-dir \
  --out analysis/reference_pools/generated \
  --pool-name "Generated Reference Pool" \
  --genre "Pop" \
  --max-files 200
```

### Build reference pool from `~/Music`

```bash
DawAI --build-reference-pool ignored --scan-music-home \
  --out analysis/reference_pools/local_music \
  --pool-name "Local Music Pool" \
  --genre "Mixed"
```

### Advisory chat request (headless)

```bash
DawAI --chat-ask "What should I fix first?" \
  --analysis analysis \
  --genre "EDM" \
  --conversation "mix-001" \
  --attach "/path/file1.txt,/path/file2.md"
```

## B) Build/install/release scripts

### Linux

```bash
./scripts/build_release.sh 1.0.0-alpha
./scripts/install_local_linux.sh linux-release Release
./scripts/install_from_archive_linux.sh /path/to/v1.0.0-alpha-archlinux-x64.tar.gz
./scripts/sync_android_truth_assets.sh
```

### Windows (PowerShell)

```powershell
.\scripts\build_release.ps1 -Version 1.0.0-alpha
.\scripts\install_local_windows.ps1 -Preset windows-release -Config Release
.\scripts\install_from_archive_windows.ps1 -ArchivePath C:\path\to\v1.0.0-alpha-windows-x64.zip
```

### Reference ingestion utilities

```bash
python scripts/fetch_archive_references.py --out-dir assets/reference_intake/archive --genres "pop,edm,rap,hip hop,trap" --per-genre 10 --pages 10 --rows 40
python scripts/fetch_openverse_references.py --out-dir assets/reference_intake/openverse --genres "pop,edm,rap" --per-genre 10 --max-pages 25
python scripts/convert_audio_tree_to_wav.py --in-dir assets/reference_intake/archive --out-dir assets/reference_intake/archive_wav
```

## C) Android Admin command templates (catalog in app)

Source: `apps/dawai_admin_android/app/src/main/java/com/dawai/admin/model/CommandCatalog.kt`

These templates are shown in admin UX and expected to execute through the daemon/bridge layer:

```bash
dawai doctor
dawai analyze run --in /path/track.wav --mode analyze --pool /path/pool --out /path/report.json
dawai analyze batch --in-dir /path/audio --glob "*.wav" --pool /path/pool --out-dir /path/reports
dawai analyze validate --report /path/report.json --schema /path/report.schema.json
dawai analyze diff --a /path/reportA.json --b /path/reportB.json --out /path/diff.json
dawai pool rebuild --pool /path/pool --sr 48000 --stft 2048 --hop 512 --truepeak 4x --frame_hz 10
dawai pool validate --pool /path/pool --deep
dawai --build-reference-cache /path/pool.json
dawai chat ask --prompt "What should I fix first?" --context /path/report.json
git -C /workspace/Vsttest status -sb
git -C /workspace/Vsttest pull
git -C /workspace/Vsttest log --oneline -n 20
```

## D) Canonical ecosystem command contract (target surface)

The system map/blueprint command contract includes namespaces for:

- `dawai version`, `dawai doctor`, `dawai daemon ...`
- `dawai files ...`, `dawai sync ...`
- `dawai analyze ...`
- `dawai pool ...`
- `dawai snapshot ...`
- `dawai build ...`, `dawai test ...`, `dawai perf ...`
- `dawai chat ...`
- `dawai device ...`

Design intent: desktop and Android admin share this same command vocabulary.

## E) Operator Notes

1. For production automation, prefer commands in section A + section B because they are implemented in this repository today.
2. Section C templates are part of admin UX and may depend on daemon/bridge availability.
3. Label command maturity clearly in internal and user docs: implemented vs contract/roadmap.
