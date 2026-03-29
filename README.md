# AIFR3D 2.2.4 Beta

AIFR3D 2.2.4 Beta is the current canonical build in this repository.

## Release Scope

- JUCE VST3 plugin
- JUCE standalone app
- website store, catalog, and support surface
- Android admin app
- OpenAI-backed AIFR3D chat and advisory path

## Build Rules

- GitHub Actions packaging is removed from this repo.
- Linux installers are provided from tracked repo files.
- Windows binaries are not packaged in GitHub.
- Windows users must clone this repo on Windows and compile locally.

## Desktop Analysis Contract

- Live scoring uses realtime DSP analysis from the audio buffer.
- Spectral balance scoring uses six FFT-derived bands:
  - Sub `20-60 Hz`
  - Low `60-200 Hz`
  - Low-Mid `200-800 Hz`
  - Mid `800 Hz-2 kHz`
  - High-Mid `2-6 kHz`
  - High `6-16 kHz`
- The detected genre mean defines the visible target window at `±3`.
- Reference comparison is measured against the nearest individual track, not a flat pooled average.
- Loudness, dynamics, stereo, and spectral metrics all feed the live score and fix list.

## AIFR3D Chat Contract

- Provider: OpenAI only
- Primary model: `gpt-5.2`
- Secondary admin-only model option: `gpt-5-nano`
- No Ollama
- No local inference
- No silent fallback
- If the OpenAI key is missing, the UI shows an explicit error state

Exact key file path used first:

- `/home/north3rnlight3r/Documents/API_KEY_INDEX/OpenAI API Key.txt`

## Linux Build

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug -j8
ctest --preset linux-debug --output-on-failure
```

## Windows Build

Windows binaries are source-built on Windows only.

1. Clone this repository on a Windows machine.
2. Open PowerShell in the repo root.
3. Run:

```powershell
./scripts/install_windows.ps1 -Target all
```

Manual configure/build:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Windows build notes:

- no Windows installer zip is published from this repo
- no GitHub Actions Windows packaging is used
- users build the Windows binary locally on Windows

## Website Release Surface

The website now exposes only 2.2.4 Beta materials:

- Linux plugin installer
- Linux standalone installer
- 2.2.4 Beta README
- installation instructions
- minimum PC requirements
- release log
- VST guide
- standalone guide

## Support

- Contact: `north3rnlight3rofficial@outlook.com`
- PayPal: `north3rnlight3rofficial@outlook.com`

## Documentation

- `docs/installation_instructions_v2.2.4_beta.md`
- `docs/release_log_v2.2.4_beta.md`
- `docs/user_manual.md`
- `docs/vst_user_guide_v2.2.4.md`
- `docs/standalone_user_guide_v2.2.4.md`
- `docs/system_requirements_v2.2.4.md`
- `docs/windows_build_guide_v2.2.4_beta.md`
- `docs/dsp_metric_registry_v2.2.4_beta.md`
- `docs/function_map.yaml`
