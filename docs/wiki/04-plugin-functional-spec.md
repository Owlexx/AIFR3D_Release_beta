# AIFR3D Plugin Functional Specification

## Scope
The plugin target is `aifred_vst3` under `Source/`.

Core files:

- Processor: `Source/Plugin/PluginProcessor.h/.cpp`
- Editor: `Source/Plugin/PluginEditor.h/.cpp`
- Parameters: `Source/Plugin/Parameters.h/.cpp`
- Runtime DSP: `Source/DSP/*`
- UI components: `Source/UI/*`

## Audio Processor Responsibilities

`DawAiAudioProcessor` responsibilities:

- host bus negotiation (mono/stereo)
- lifecycle hooks (`prepareToPlay`, `releaseResources`)
- per-block deterministic analysis tap
- APVTS state serialization/deserialization
- lock-free frame publishing to UI (`uiFifo`)

Important behavior in `processBlock`:

1. compute timestamp (`tSec`)
2. run `AnalysisEngine::process(...)`
3. push `AnalysisFrame` to UI FIFO

No advisory/network/disk operations occur here.

## Analysis Engine Responsibilities

`Source/DSP/AnalysisEngine.cpp` orchestrates:

- feature extraction
- loudness / true-peak
- spectrum tilt
- genre profile detection and stabilization
- scoring + confidence
- event flag derivation

Outputs are written into `AnalysisFrame` for UI consumption.

## Genre Detection (Current Behavior)

Current runtime classifier uses nearest-profile distance against profile means in `ReferenceModel`.

- target genres: Pop, EDM, HipHop/Trap
- confidence derived from inverse distance
- hysteresis/stability logic avoids rapid switching

Profile means can be updated by loading reference profile JSONs from:

- default: `assets/reference_pools/archive_10x5/profiles`
- override: `DAWAI_REFERENCE_PROFILE_DIR`

## UI System and Modes

UI root: `Source/UI/MainView`.

Main visual units:

- `TopBar` (status, mode toggles, genre/confidence line)
- `LifeMeter` (creative vitality)
- `ApprovalHalo` (segment ring + overall score)
- `CandleStoryline` (dynamics timeline)
- `StereoArcView` (spatial state)
- `FixListPanel` (ranked guidance cards)

`UiModel` holds smoothed render state and derives UI-level status text from frame values and event flags.

## State and Preset Handling

Processor state is stored via APVTS XML.

- Save: `getStateInformation`
- Restore: `setStateInformation`

This preserves plugin settings and avoids ad-hoc state blobs.

## Output and Reporting Surface

The VST UI displays live analysis state. Deeper reporting contracts are backed by:

- `Source/DSP/JsonReportWriter.*`
- `docs/schema/report.schema.json`

## Realtime Safety Guarantees

- lock-free queue boundary (`SimpleLockFreeQueue`)
- no blocking calls in `processBlock`
- no dynamic workflow operations in callback

## Packaging Targets
CMake generates:

- VST3 bundle: `AIFRED.vst3`
- standalone app: `AIFRED`

Install scripts:

- Linux local install: `scripts/install_local_linux.sh`
- Linux archive install: `scripts/install_from_archive_linux.sh`
- Windows local install: `scripts/install_local_windows.ps1`
- Windows archive install: `scripts/install_from_archive_windows.ps1`
