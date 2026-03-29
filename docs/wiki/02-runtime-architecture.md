# Runtime Architecture

## Layered Module Graph

```text
metering
   -> reference_engine
      -> aifr3d_core
         -> advisory_layer

audio_engine -> (uses metering/reference outputs)
ui           -> (renders outputs + user intents)
```

Rules:

- No circular dependencies
- No god object
- Advisory layer cannot mutate deterministic analysis outputs

Reference: `docs/adr/0001-module-boundaries.md`

## Threading and Realtime Safety

### Audio thread
Responsibilities:

- capture/process audio blocks
- run lightweight per-block feature extraction
- publish `AnalysisFrame` snapshots to lock-free queue

Forbidden on audio thread:

- disk I/O
- JSON parsing
- network calls
- blocking locks

### Analysis worker
Responsibilities:

- heavier aggregate calculations
- reference profile comparisons
- scoring and fix-list derivation

### Advisory worker
Responsibilities:

- advisory rewrite from deterministic outputs
- local model invocation when enabled

### UI thread
Responsibilities:

- consume latest frames
- smooth UI values
- render and interact with user controls

Reference: `docs/threading_model.md`

## Data Contracts

### Realtime frame contract
`Source/Common/AnalysisTypes.h` defines `AnalysisFrame`:

- core scores (`lifeIndex`, `approval`, segment scores)
- loudness/peak (`integratedLUFS`, `shortTermLUFS`, `truePeakDbTP`)
- stereo state (`midEnergy`, `sideEnergy`, `correlation`, `phaseRisk`)
- dynamic candle payload (`dynOpen`, `dynClose`, `dynHigh`, `dynLow`)
- event flags (`kFlag*`)
- genre + confidence (`genreIndex`, `genreConfidence`)

### Persisted report contract
`docs/schema/report.schema.json` defines analysis report structure:

- session metadata
- reference context
- metric groups
- timeline candles
- fix list
- event log

## Startup and First-Run Flow
`apps/dawai/src/main.cpp`:

1. starts logging (`logs/`)
2. parses CLI flags for batch/headless operations
3. if GUI mode, ensures `config/first_run.json` exists
4. opens main window (`dawai::ui::MainView`)

## Failure Containment Strategy

- Missing reference pool: system still runs with fallback defaults
- Advisory unavailable: deterministic analysis still runs
- Profile parse failures: skip bad profile entries, continue pipeline
- Plugin state corruption prevention: APVTS XML state load guarded by tag checks

## Why This Scales

- independent modules can evolve in parallel
- deterministic contracts allow regression testing at multiple layers
- additional surfaces (CLI service, cloud jobs) can reuse the same report schema
