# AIFR3D Standalone User Guide 2.2.4 Beta

## Purpose

The standalone app provides the same analysis core as the plugin without requiring a DAW host.

## Install

### Linux

Run the tracked installer script:

```bash
./apps/website/downloads/standalone-linux-installer.sh
```

### Windows

Windows users build locally on Windows:

```powershell
./scripts/install_windows.ps1 -Target standalone
```

No prebuilt Windows standalone binary is published from this repo.

## Core Views

- main diagnostic view
- visual analysis view
- session progression view
- advanced metrics view

## Persistent Behavior

- fix output persists in-session
- chat history persists in-session
- session trend data persists between runs

## Chat

- provider: OpenAI
- primary model: `gpt-5.2`
- no local fallback
- key path: `/home/north3rnlight3r/Documents/API_KEY_INDEX/OpenAI API Key.txt`
