# AIFR3D Full Forensic Build Manual

Date: 2026-03-26
Repository: `/home/north3rnlight3r/Projects/AifredVst-2.2.4-beta`

## What This Document Is

This is a full build schema, not a short summary.

It is written for someone who may not code much but still needs to understand:

- what this repository contains
- how the plugin, standalone app, Android admin app, website, backend, and worker fit together
- how the DSP analysis path works
- how the three UI modes differ
- how the halo systems and meters are driven
- how to build each deliverable from zero
- which files matter most
- what each major build command does
- what outputs you should expect
- where the system is still hard-coded to `2.2.4`

Important truth:

- The repository folder name and many source files still identify the product as `2.2.4 Beta`.
- If you are treating this as a `2.2.5` continuation, the build architecture is still here, but version strings and some install labels still need a later version-bump pass.

## Plain-English System Map

This repository contains several products that share one brand and one analysis ecosystem.

### 1. Desktop plugin and standalone analysis app

These are the JUCE/C++ products.

- VST3 plugin variants live under `Source/`
- JUCE GUI app lives under `apps/dawai`
- A small standalone CLI demo lives under `apps/dawai_standalone`
- Shared C++ engine code also exists under `cpp/` and `modules/`

### 2. Android admin app

This is a Kotlin + Jetpack Compose Android project.

- Path: `apps/android_admin`

It talks to the backend and acts like a private control room for:

- admin login
- uploads
- commands
- chat
- website control

### 3. Website

This is a static frontend with a backend API.

- Static public site: `apps/website`
- Local backend: `server/src/index.js`
- Cloudflare Worker deploy surface: `cloudflare/website-worker.mjs`
- Wrangler config: `wrangler.toml`

### 4. Shared analysis logic

This is the instrument heart of the plugin.

The key path is:

- audio enters the plugin processor
- the audio buffer is passed into `AnalysisEngine`
- the engine preprocesses and measures the buffer
- the results are stored in `AnalysisFrame`
- the `UiModel` smooths and formats those results for display
- the UI components paint the already-measured values

That matters because the GUI is not supposed to invent movement or run heavy analysis math in paint calls.

## The Most Important Build Files

These files define how the project builds.

### Root CMake

File: `CMakeLists.txt`

This is the main CMake entry point.

Key code:

```cmake
cmake_minimum_required(VERSION 3.24)
project(DawAI VERSION 2.2.4 LANGUAGES C CXX)

option(DAWAI_BUILD_APP "Build the DAW executable" ON)
option(DAWAI_BUILD_AIFRED_PLUGIN "Build the AIFRED VST3 plugin target" ON)
option(DAWAI_ENABLE_TESTS "Build unit tests" ON)

add_subdirectory(modules/reference_engine)
add_subdirectory(modules/aifr3d_core)
add_subdirectory(modules/advisory_layer)
add_subdirectory(modules/audio_engine)
add_subdirectory(modules/ui)

if(DAWAI_BUILD_APP)
    add_subdirectory(apps/dawai)
endif()

if(DAWAI_BUILD_AIFRED_PLUGIN)
    add_subdirectory(Source)
endif()
```

What this means in plain English:

- `project(...)` names the C++ project and version
- the `option(...)` lines are build switches
- `add_subdirectory(...)` tells CMake which folders contain buildable code
- if plugin building is enabled, `Source/` is compiled
- if app building is enabled, `apps/dawai` is compiled

### Plugin CMake

File: `Source/CMakeLists.txt`

This file defines the JUCE plugin targets.

Key code:

```cmake
juce_add_plugin(${target}
    COMPANY_NAME "North3rnLight3r"
    IS_SYNTH FALSE
    FORMATS VST3 Standalone
    PRODUCT_NAME "AIFRED_${suffix}")
```

What this means:

- JUCE creates plugin targets
- each target builds both a `VST3` artifact and a `Standalone` artifact
- the repo creates four named variants:
  - `aifred_vst3_A`
  - `aifred_vst3_B`
  - `aifred_vst3_C`
  - `aifred_vst3_D`

So when you build the plugin target, you are really building multiple branded variants.

### Build presets

File: `CMakePresets.json`

This gives ready-made preset names.

Example:

```json
{
  "name": "linux-debug",
  "generator": "Ninja",
  "binaryDir": "${sourceDir}/build/linux-debug",
  "cacheVariables": {
    "CMAKE_BUILD_TYPE": "Debug",
    "DAWAI_ENABLE_TESTS": "ON",
    "DAWAI_BUILD_APP": "ON"
  }
}
```

What this means:

- a preset is a saved build configuration
- `linux-debug` means:
  - use Ninja
  - build inside `build/linux-debug`
  - use debug mode
  - enable tests
  - build the app too

### Node backend package

File: `package.json`

Key code:

```json
{
  "scripts": {
    "dev": "node server/src/index.js",
    "start": "node server/src/index.js"
  }
}
```

What this means:

- `npm run dev` and `npm start` both launch the same backend file
- the backend is not a React/Vite/Next app
- it is a plain Node server using Express

### Android Gradle app config

File: `apps/android_admin/app/build.gradle.kts`

Key code:

```kotlin
android {
    namespace = "com.dawai.admin"
    compileSdk = 35

    defaultConfig {
        applicationId = "com.dawai.admin"
        minSdk = 29
        targetSdk = 35
        versionCode = 224
        versionName = "2.2.4"
    }
}
```

What this means:

- the Android package name is `com.dawai.admin`
- Android SDK 35 is the target
- minimum supported Android version is API 29
- the mobile app version is still `2.2.4`

### Website worker config

File: `wrangler.toml`

Key code:

```toml
name = "north3rnlight3r-website"
main = "cloudflare/website-worker.mjs"

[assets]
directory = "./apps/website"
binding = "ASSETS"
```

What this means:

- Cloudflare deploys the Worker script
- the worker serves the static website folder as bound assets

## Directory Forensics

This is the practical repo map.

### Plugin and DSP

- `Source/Plugin`:
  - audio processor and editor
- `Source/DSP`:
  - loudness
  - FFT
  - stereo analysis
  - feature extraction
  - scoring
  - reference model
- `Source/UI`:
  - halo
  - fingerprint
  - candles
  - stereo view
  - fix list
  - top bar
  - UI model
- `Source/Common`:
  - shared metric structs

### App and modules

- `apps/dawai`: JUCE GUI app target
- `modules/`: shared modular code used by multiple app surfaces

### Android

- `apps/android_admin`

### Website and backend

- `apps/website`: public static site
- `server/src/index.js`: local backend
- `cloudflare/website-worker.mjs`: edge worker
- `deploy/`: production reverse proxy and service examples

### Tests

- `tests/`: C++ tests for pipeline, metering, scoring, and host smoke coverage

## The Real DSP Path

This is the most important part of the plugin.

The relevant file is:

- `Source/DSP/AnalysisEngine.cpp`

## Step-by-Step DSP Flow

### 1. Audio buffer enters `AnalysisEngine::process`

Relevant code shape:

```cpp
bool AnalysisEngine::process(const juce::AudioBuffer<float>& buffer, double tSec,
                             AnalysisFrame& out)
{
    preprocessMeasurementBuffer(buffer, m_measurementBuffer);

    auto spectral = m_spectrum.analyze(m_measurementBuffer);
    auto features = m_featureExtractor.extract(m_measurementBuffer, &spectral);
    const auto stereoField = m_stereo.analyze(m_measurementBuffer);
    auto loudness = m_loudness.process(m_measurementBuffer);
}
```

Plain English:

- the raw buffer comes in
- the engine first prepares a measurement-safe copy
- then it measures:
  - spectrum
  - features
  - stereo field
  - loudness

### 2. DC offset is removed before measurement

This happens in `preprocessMeasurementBuffer(...)`.

The current code:

- calculates the mean of each channel
- subtracts that mean from each sample

That is a DC-removal step.

Why it matters:

- DC offset can distort RMS, correlation, and other energy-based metrics
- removing it first makes the instrument more trustworthy

### 3. FFT/spectral analysis

File: `Source/DSP/SpectrumAnalyzerFFT.cpp`

This analyzer:

- uses FFT order `12`
- meaning FFT size `4096`
- uses a Hann window
- converts the mono summed buffer to the frequency domain
- groups energy into bands

The measured bands include:

- Sub: `20-60 Hz`
- Low: `60-120 Hz`
- Low-Mid: `120-400 Hz`
- Mid: `400-2000 Hz`
- High-Mid: `2000-6000 Hz`
- High: `6000-12000 Hz`
- Presence: `4000-8000 Hz`
- Air: `12000 Hz` to Nyquist limit up to `20 kHz`

It also calculates spectral tilt using linear regression over log-frequency.

Why that matters:

- band values drive tonal displays
- spectral tilt helps describe bright vs dark balance
- the halo and fingerprint both depend on this

### 4. Loudness and true peak

File: `Source/DSP/LoudnessEBUR128.cpp`

This is the loudness core.

It does:

- K-weighting
- short-term loudness
- momentary loudness
- integrated loudness with gating
- true peak using oversampling
- sample peak dBFS

Important code behavior:

- high-shelf and high-pass filters are used for K-weighting
- true peak uses `juce::dsp::Oversampling<float>` with 4x oversampling

Why that matters:

- this is the most engineering-correct part of the current analyzer stack
- the loudness instrument is not a fake bar; it comes from actual weighted energy and oversampled peak detection

### 5. Feature extraction

File: `Source/DSP/FeatureExtractor.cpp`

This computes:

- RMS
- peak
- crest factor
- transient rate
- transient density
- mid energy
- side energy
- correlation
- tonal helper descriptors like mud/harshness/air

Current warning:

- some of the mud/harshness/air logic is still a higher-level heuristic layer, not purely textbook measurement
- the transient density is also normalized for UI convenience

That does not make it useless, but it does mean a strict `2.2.5` forensic DSP refactor should revisit this file first.

### 6. Stereo analysis

File: `Source/DSP/StereoAnalyzer.cpp`

This calculates:

- left energy
- right energy
- mid energy
- side energy
- stereo width
- correlation
- low-band correlation
- low-band phase risk
- center dominance
- side dominance
- stereo motion
- WXYZ-style axes

Important mapping:

- `W` = mid energy
- `X` = left energy
- `Y` = right energy
- `Z` = side energy

This is the spatial field instrument.

### 7. Reference classification

File: `Source/DSP/ReferenceModel.cpp`

This layer does not replace live measurements.

It supplies:

- target corridors
- reference means
- nearest profile match
- loudness status windows
- dynamic range status windows
- spectral target bands

That means:

- the measured value stays live
- the reference changes interpretation, not the raw meter number

### 8. Scoring and flags

Files:

- `Source/DSP/ScoringEngine.cpp`
- `Source/DSP/AnalysisEngine.cpp`

This layer generates:

- behavior index
- mix alignment
- signal clarity
- signal stability
- segment scores for tone, dynamics, space, punch, balance
- issue flags like:
  - harshness burst
  - phase risk
  - clip risk
  - width collapse
  - transient softening

Important principle:

- measurement comes first
- scoring happens after measurement

## How the Meters Are Driven

The main UI mediator is:

- `Source/UI/UiModel.cpp`

This is the bridge between DSP and paint.

The GUI does not directly re-measure audio.

Instead:

1. `AnalysisEngine` fills an `AnalysisFrame`
2. `MixMetrics::fromAnalysisFrame(...)` converts it into a structured metric object
3. `UiModel::updateFrom(...)` copies the values into smoothed UI fields
4. `UiModel::advance()` applies motion smoothing
5. the paint code reads the already-smoothed values

That separation is what makes the GUI feel live without forcing expensive DSP math into the paint loop.

### Example: meter ballistic behavior

In `UiModel::advance()`:

```cpp
meterShortTermLUFS = ballisticFollow(meterShortTermLUFS,
                                     hasLiveSignal ? shortTermLUFS : quietLufs, m_uiHz, 110.0f,
                                     420.0f);
```

Plain English:

- `meterShortTermLUFS` is not redrawn by jumping instantly to the raw value
- it follows the raw loudness with a controlled attack and release
- this makes it readable
- the source is still the real measured value

### Smoothed display values currently include

- short-term LUFS
- momentary LUFS
- true peak
- sample peak
- crest factor
- stereo width
- correlation
- volatility
- live band dB rows
- reference delta rows
- tone/stereo/loudness/dynamics summary meters

## The Three Modes

The mode enum lives in `Source/UI/UiModel.h`:

```cpp
enum class UiPresentationMode
{
    Analyze,
    Compare,
    Reference
};
```

Mode switching happens in `Source/UI/MainView.cpp` through the top bar.

## Mode 1: Analyze

Purpose:

- show the live mix as a measurement instrument

What it uses:

- current realtime buffer
- current DSP metrics

What it does not depend on:

- compare snapshots
- reference target for the raw measured values

Behavior:

- meters move from the live buffer
- fingerprint shows live mix identity
- halo shows live tone, width, peak, and punch
- cards and band rows show current live values

In code:

- `model.setPresentationMode(UiPresentationMode::Analyze);`
- `MixSignatureMeter` draws live fingerprint only
- `ApprovalHalo` uses:
  - `liveToneMeter`
  - `liveStereoMeter`
  - `liveLoudnessMeter`
  - `liveDynamicsMeter`

Analyze mode is the most instrument-like mode.

## Mode 2: Compare

Purpose:

- compare one captured mix state to another

What it uses:

- `compareMixA`
- `compareMixB`
- if B is not captured, the live buffer can act as live B

Behavior:

- the fingerprint overlays A vs B
- the halo switches from absolute live instrument logic to delta logic
- bands are shown as A/B differences, not reference drift

In `ApprovalHalo.cpp`, compare mode segment labels become:

- `TONE Δ`
- `WIDTH Δ`
- `PEAK Δ`
- `PUNCH Δ`

This matters:

- Analyze mode asks, "what is my mix doing right now?"
- Compare mode asks, "how is Mix B different from Mix A?"

Important current forensic note:

- the current repo compare mode uses one compare halo panel with delta behavior
- your local design notes in `Documents/aifredmergeplan.txt` describe a future dual-halo compare concept
- that means the design intent is broader than the currently shipped repo implementation

## Mode 3: Reference

Purpose:

- compare live measurements against a target corridor

What it uses:

- live measurement values from the buffer
- active reference profile and corridor targets

What it does not do:

- replace the raw measured values with target values

Behavior:

- live meters remain live
- colors and ghost overlays show corridor fit
- the fingerprint may show a purple target ghost polygon
- the halo shows live segment length with reference-aware color evaluation

This is the mastering / alignment mode.

## The Halo Systems

There are effectively two halo-related visual systems in the current plugin surface.

### Halo system 1: the main segmented diagnostic halo

File: `Source/UI/ApprovalHalo.cpp`

This is the ring-based central status instrument.

It contains four primary segments.

By design and code, the four core areas map to:

- Tone
- Stereo / Width
- Loudness / Peak
- Dynamics / Punch

In Analyze mode, these are drawn from live instrument meters.

In Reference mode:

- arc length still shows live measurement intensity
- arc color reflects reference corridor evaluation

In Compare mode:

- segment labels become deltas
- segment amount reflects magnitude of A/B difference
- color reflects whether B is lower, similar, or hotter/wider than A

### Halo system 2: the pulse/spectrum layer inside and around the halo

This is not a completely separate DSP engine.

It is the motion and overlay language around the halo:

- outer pulse emphasis
- inner spectrum line
- center text
- footer and mode explanation

The code for this is also inside `ApprovalHalo.cpp`.

What it does:

- draws a spectrum line using live band values
- optionally overlays compare or reference spectral context
- changes center text depending on mode
- changes footer legend depending on mode

So when you say "two halos," the clearest forensic interpretation in the current repo is:

1. the segmented halo ring itself
2. the pulse/spectrum/center-readout halo layer that makes the ring feel alive and mode-aware

Important future-design note:

- your merge-plan notes also describe a compare-mode future with literal dual halo instances side by side
- that is a valid next architecture step, but it is not the same thing as the single-halo compare implementation currently in this repo

## How the Halo Is Driven

### In Analyze mode

The segment values are pulled from live UI meter values:

- `liveToneMeter`
- `liveStereoMeter`
- `liveLoudnessMeter`
- `liveDynamicsMeter`

These themselves come from DSP-derived metrics such as:

- spectral bands and spectral tilt
- stereo width and correlation
- short-term LUFS and true peak
- crest factor and transient density

### In Reference mode

The segment lengths remain live.

The segment colors are decided by reference-aware functions such as:

- `toneReferenceColour(...)`
- `stereoReferenceColour(...)`
- `loudnessReferenceColour(...)`
- `dynamicsReferenceColour(...)`

This is a critical architectural rule:

- live number = measurement
- color = interpretation against target corridor

### In Compare mode

The halo uses A/B deltas.

Examples from the code:

- tone delta is the average spectral difference across the main bands
- width delta is `mixB.width - mixA.width`
- peak delta is `mixB.truePeak - mixA.truePeak`
- punch delta is `mixB.crest - mixA.crest`

## The Mix Signature / Fingerprint System

File: `Source/UI/MixSignatureMeter.cpp`

This is the second major analysis panel, but unlike the segmented halo, it is not a four-segment status ring.

It is a 12-axis fingerprint.

Current axis labels in code:

- `SUB`
- `CONTROL`
- `LOW-MID`
- `MID`
- `BITE`
- `AIR`
- `DYN`
- `TRNS`
- `WIDTH`
- `CENTER`
- `PHASE`
- `PRESS`

These map to real measured or derived metric families, for example:

- Sub = sub band dB
- Control = low-end control from sub-mono integrity and low-band phase risk
- Low-Mid = low-mid band dB
- Mid = mid band dB
- Bite = high-mid to mid relationship
- Air = air band plus spectral tilt
- Dyn = crest factor
- Trns = transient rate
- Width = stereo width
- Center = center dominance
- Phase = correlation + sub mono stability
- Press = integrated LUFS pressure

This is not supposed to be decorative art.

It is a compressed geometric summary of the mix’s identity.

## Candlestick Systems

The UI model stores two candle families.

From `UiModel.h`:

- `candleBuf`: session candles
- `realtimeCandleBuf`: realtime candles

What they represent:

- session candles: progress over mix sessions
- realtime candles: short-window motion inside the live session

The values include:

- open
- close
- high
- low
- and deviation fields for:
  - LUFS
  - true peak
  - crest
  - spectral tilt
  - width
  - correlation
  - transient behavior

This is why the candle panel is more than a decorative history chart.

## Volatility

Volatility is tracked inside `AnalysisEngine.cpp` and surfaced through `UiModel`.

The engine stores recent points including:

- momentary LUFS
- short-term LUFS
- crest dB
- spectral tilt
- width
- correlation
- transient density
- true peak

Then it computes a volatility breakdown.

The purpose:

- detect instability and motion across recent time windows
- give the UI a readable "how unstable is this mix behavior right now?" value

## Plugin Build: Linux From Zero

This section assumes a Linux machine.

## Step 1: open the repo root

```bash
cd /home/north3rnlight3r/Projects/AifredVst-2.2.4-beta
```

Why:

- almost every script in this repo expects you to be in the root directory

## Step 2: install the basic dependencies

The repo’s own Linux installer script checks for these:

- `curl`
- `node`
- `npm`
- `cmake`
- `g++`
- `make`
- `python3`

And also Linux audio/UI dev packages such as:

- `pkg-config`
- `libasound2-dev`
- `libx11-dev`
- `libxrandr-dev`
- `libxinerama-dev`
- `libxcursor-dev`
- `libxcomposite-dev`
- `libxext-dev`
- `libfreetype6-dev`
- `libgtk-3-dev`

If you want the repo to try auto-installing them on Debian/Ubuntu-based systems, use:

```bash
bash scripts/install_local_linux.sh
```

What that script does:

1. checks system packages
2. runs `npm install`
3. runs `cmake -S . -B build`
4. builds the A/B/C/D plugin targets
5. verifies backend syntax
6. performs a local backend health check
7. installs standalone binaries and/or VST3 bundles into user folders

If you want to understand and do the steps manually instead, continue below.

## Step 3: install Node packages

```bash
npm install
```

Why:

- the website/backend/admin surfaces rely on Node dependencies in `package.json`
- even if you only care about the plugin, several helper scripts and the local ecosystem expect the backend dependencies to exist

## Step 4: configure CMake

Option A: use presets

```bash
cmake --preset linux-debug
```

Option B: basic manual configure

```bash
cmake -S . -B build
```

What this does:

- creates the build system files
- downloads JUCE and other dependencies through CPM
- prepares native compile targets

## Step 5: build the plugin/app

Using presets:

```bash
cmake --build --preset linux-debug -j8
```

Manual target build:

```bash
cmake --build build --target aifred_vst3_A aifred_vst3_B aifred_vst3_C aifred_vst3_D --parallel
```

What you should expect:

- build folders under `build/`
- plugin artifacts under `build/Source/aifred_vst3_*_artefacts/`
- standalone artifacts beside each plugin variant artifact tree

## Step 6: run tests

Preset way:

```bash
ctest --preset linux-debug --output-on-failure
```

Tests currently present include:

- canonical pipeline smoke
- metering signal test
- plugin host smoke
- spectral pipeline smoke
- report generation
- session serialization

Why this matters:

- if you changed DSP or build logic, these are the first protection layer against silent regressions

## Step 7: install VST3 and standalone outputs

The Linux install script copies artifacts to:

- VST3: `~/.vst3`
- standalone binaries: `~/.local/share/dawai/bin`
- symlinks: `~/.local/bin`

If you want that automated:

```bash
bash scripts/install_local_linux.sh
```

If you want only VST:

```bash
DAWAI_INSTALL_TARGET=vst bash scripts/install_local_linux.sh
```

If you want only standalone:

```bash
DAWAI_INSTALL_TARGET=standalone bash scripts/install_local_linux.sh
```

## Plugin Build: Windows From Zero

This repo’s current rule is:

- build Windows binaries on Windows
- do not expect Linux to produce the normal Windows result unless you have a deliberate cross-toolchain setup

## Step 1: install Windows tools

Required:

- Windows 10 or 11 x64
- Visual Studio 2022 with Desktop development with C++
- CMake 3.24+
- Node.js LTS
- Git

## Step 2: open PowerShell in the repo root

```powershell
cd C:\path\to\AifredVst-2.2.4-beta
```

## Step 3: automated build/install

```powershell
./scripts/install_windows.ps1 -Target all
```

What it does:

1. installs `node` and `cmake` through `winget` if missing
2. runs `npm install`
3. configures a Visual Studio 2022 build
4. builds the four plugin targets
5. copies VST3 bundles into the FL Studio VST folder
6. copies standalone executables into the local app folder
7. installs the payment proof bundle

## Step 4: manual Windows build if needed

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

## Where Windows outputs go

The script installs to:

- VST3 destination:
  - `%USERPROFILE%\Documents\Image-Line\FL Studio\Plugins\VST`
- standalone destination:
  - `%LOCALAPPDATA%\Programs\DawAI\bin`

## Android Admin App Build From Zero

Project path:

- `apps/android_admin`

## Step 1: install Android prerequisites

You need:

- Android Studio or Android command-line SDK tools
- Java 17
- Android SDK platform 35
- Android build tools 35
- ADB

## Step 2: create or update `local.properties`

Inside `apps/android_admin/local.properties`, set at minimum:

```properties
sdk.dir=/home/yourname/Android/Sdk
dawaiBaseUrl=https://www.north3rnlight3r.com
dawaiApiToken=
dawaiAdminUsername=YOUR_ADMIN_USERNAME
dawaiAdminPassword=YOUR_ADMIN_PASSWORD
```

Why:

- `sdk.dir` tells Gradle where the Android SDK is
- `dawaiBaseUrl` tells the app which backend to talk to
- username/password can be injected into build config for admin convenience

Important:

- the current Gradle file contains hard-coded default admin fallback values in source
- do not rely on those in a serious deployment
- override them locally instead

## Step 3: build debug APK

```bash
cd apps/android_admin
./gradlew :app:assembleDebug
```

What this does:

- resolves Gradle plugins
- compiles Kotlin and Compose UI
- packages the Android app into an APK

Expected output:

- `apps/android_admin/app/build/outputs/apk/debug/app-debug.apk`

## Step 4: install to a connected Android device

Automated script:

```bash
bash scripts/install_android.sh
```

What it does:

1. checks for Android SDK
2. writes `sdk.dir` into local properties
3. injects optional admin credentials from env vars
4. builds the debug APK
5. runs `adb install -r`

If you want to pass credentials through the script:

```bash
export DAWAI_ADMIN_USERNAME="your_admin_user"
export DAWAI_ADMIN_PASSWORD="your_admin_password"
bash scripts/install_android.sh
```

## Local Website + Backend Build From Zero

This system has two web-serving modes:

1. local Node backend plus static assets
2. Cloudflare Worker plus static assets

## Local mode: what is running

The local backend file is:

- `server/src/index.js`

It serves:

- API routes
- admin routes
- catalog routes
- uploads
- receipts
- OpenAI-backed chat routes
- local storage-backed settings and memory

It also uses the website folder:

- `apps/website`

## Step 1: install Node dependencies

```bash
cd /home/north3rnlight3r/Projects/AifredVst-2.2.4-beta
npm install
```

## Step 2: run the backend locally

Simple dev run:

```bash
npm run dev
```

Equivalent direct run:

```bash
node server/src/index.js
```

By default the backend listens on:

- `PORT=8787`

## Step 3: verify health

```bash
curl -fsS http://127.0.0.1:8787/api/v1/health
```

If this works, the local backend is alive.

## Step 4: open the website

Because the website files are static and the backend serves the API, your local workflow is:

- run the backend
- serve or open the website assets

For beginner-safe local testing, the simplest setup is:

1. keep `node server/src/index.js` running
2. open `apps/website/index.html` in a browser if the API routes are same-origin-compatible in your environment
3. if you want a cleaner local web server, serve `apps/website` through any simple static file server while keeping the backend running

## What the backend code is doing

At the top of `server/src/index.js`, the file defines important folders like:

```js
const WEBSITE_DIR = path.join(ROOT_DIR, "apps/website");
const STORAGE_DIR = path.join(ROOT_DIR, "server/storage");
const OPENAI_CONFIG_FILE = path.join(STORAGE_DIR, "openai_config.json");
const CHAT_SETTINGS_FILE = path.join(STORAGE_DIR, "chat_settings.json");
```

This means:

- website assets are served from `apps/website`
- backend state is stored under `server/storage`
- local persistence is file-based

## Production Backend Service

There is a systemd example file:

- `deploy/dawai-backend.service`

And a runtime launcher:

- `scripts/run_backend_production.sh`

That script loads:

- OpenAI key
- PayPal client ID
- PayPal secret

from files in `~/Documents/API_KEY_INDEX/` unless env vars are already set.

Production start command:

```bash
bash scripts/run_backend_production.sh
```

## Website Deploy Through Cloudflare Worker

Worker file:

- `cloudflare/website-worker.mjs`

Wrangler config:

- `wrangler.toml`

## What the worker does

- serves static public assets from `apps/website`
- provides edge API routes
- handles chat-related routes
- exposes payment and product endpoints

## Basic deploy logic

The worker is configured with:

```toml
[assets]
directory = "./apps/website"
binding = "ASSETS"
```

This means the static site folder is bundled with the worker deployment.

## Typical deploy steps

1. install Wrangler
2. authenticate Wrangler to Cloudflare
3. make sure `wrangler.toml` points to the correct routes
4. deploy

Command shape:

```bash
wrangler deploy
```

This deploys:

- the worker code
- the static website asset bundle

## Caddy Reverse Proxy Option

The repo also contains a Caddy example:

- `deploy/Caddyfile.north3rnlight3r.example`

It shows a more traditional server path where:

- static files come from a website folder
- `/api/*`, `/ws/chat`, `/downloads/*`, `/media/*`, `/receipts/*` are reverse-proxied to the local backend on port `8787`

That means this ecosystem can operate in two styles:

1. Cloudflare edge-first
2. local/backend + reverse-proxy server

## A Beginner’s "0 To 100%" Full Build Order

If you want the least confusing order, do it exactly like this.

### Phase 0: clone and inspect

1. clone the repo
2. enter the repo root
3. confirm the important folders exist:
   - `Source`
   - `apps/android_admin`
   - `apps/website`
   - `server`
   - `scripts`

### Phase 1: desktop prerequisites

1. install CMake
2. install a C++20 compiler
3. install Ninja if using presets
4. install Node.js and npm
5. run `npm install`

### Phase 2: desktop configure/build

1. run `cmake --preset linux-debug` or manual `cmake -S . -B build`
2. build the plugin and app targets
3. run `ctest`
4. install using the helper script if desired

### Phase 3: backend validation

1. run `node --check server/src/index.js`
2. run `npm run dev`
3. verify `/api/v1/health`

### Phase 4: Android admin app

1. install Android SDK and Java 17
2. create `apps/android_admin/local.properties`
3. run `./gradlew :app:assembleDebug`
4. install with ADB or `scripts/install_android.sh`

### Phase 5: website local

1. run the backend
2. serve or open `apps/website`
3. confirm the frontend can reach backend routes

### Phase 6: website production

1. configure Cloudflare
2. verify Wrangler auth
3. deploy the worker
4. verify public routes

### Phase 7: instrument verification

1. play program material into the plugin
2. confirm the halo changes
3. confirm the fingerprint unlocks after warmup
4. confirm candles move
5. confirm compare snapshots capture
6. confirm reference mode changes color/corridor behavior without freezing live values

### Phase 8: admin verification

1. log into the Android app
2. test admin login
3. test file upload
4. test command route
5. test website control paths

### Phase 9: release hardening

1. verify version strings
2. verify asset paths
3. verify key paths
4. verify payment config
5. verify website download links
6. verify logs and storage directories

## Common Confusions Explained

### "Why is there both a backend and a Cloudflare Worker?"

Because the repo is carrying both:

- a richer local file-backed backend
- an edge-deployed web surface

They overlap in purpose, but they are not identical.

### "Why does the plugin seem different from the website/backend?"

Because the plugin is a JUCE C++ application using CMake and DSP code, while the website/admin side is JavaScript/Node/Cloudflare/Android.

They are one product family, not one programming stack.

### "Why are there still 2.2.4 strings everywhere if this is a 2.2.5 continuation?"

Because the repo state on 2026-03-26 still hardcodes `2.2.4` in many places:

- root CMake version
- Android app version
- installer scripts
- product labels
- worker pricing/version text
- user docs

That means:

- the build architecture is usable
- the branding/version pass is still separate work

## Files You Should Read First If You Continue Development

For plugin/DSP:

- `Source/DSP/AnalysisEngine.cpp`
- `Source/DSP/LoudnessEBUR128.cpp`
- `Source/DSP/SpectrumAnalyzerFFT.cpp`
- `Source/DSP/FeatureExtractor.cpp`
- `Source/DSP/StereoAnalyzer.cpp`
- `Source/UI/UiModel.cpp`
- `Source/UI/ApprovalHalo.cpp`
- `Source/UI/MixSignatureMeter.cpp`
- `Source/UI/MainView.cpp`

For Android:

- `apps/android_admin/app/build.gradle.kts`
- `apps/android_admin/app/src/main/java/com/dawai/admin/MainActivity.kt`

For web:

- `server/src/index.js`
- `apps/website/index.html`
- `apps/website/config.js`
- `cloudflare/website-worker.mjs`
- `wrangler.toml`

## Final Practical Advice

If you are building this ecosystem from zero and do not want to get lost:

1. get the desktop build working first
2. get the Node backend healthy second
3. build the Android app third
4. only then move to Cloudflare deployment
5. do not mix "version bump" work with "build recovery" work
6. do not change the GUI look until the DSP and data path are trustworthy

That order matches how this repository is actually structured.
