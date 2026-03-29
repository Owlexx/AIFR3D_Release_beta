# AIFR3D VST3 User Guide 2.2.4 Beta

## Purpose

AIFR3D VST3 runs inside a DAW and reacts to live tonal, stereo, loudness, and dynamics changes from the realtime buffer.

## Install

### Linux

Run the tracked installer script:

```bash
./apps/website/downloads/plugin-linux-installer.sh
```

### Windows

Windows users build locally on Windows:

```powershell
./scripts/install_windows.ps1 -Target vst
```

No prebuilt Windows VST package is published from this repo.

## Metering

- halo = high-level diagnostic state
- mix signature = main score and alignment readout
- spectral plot = live buffer vs reference context
- fix list = persistent OpenAI advisory output tied to live metrics

## What Moves The Score

- bass removal lowers sub/low spectral alignment
- low-mid buildup increases mud pressure
- upper-mid and high boosts increase harshness/brightness pressure
- stereo collapse moves width and correlation
- limiting changes loudness and dynamics balance

## Chat

- provider: OpenAI
- primary model: `gpt-5.2`
- no local fallback
- key path: `/home/north3rnlight3r/Documents/API_KEY_INDEX/OpenAI API Key.txt`
