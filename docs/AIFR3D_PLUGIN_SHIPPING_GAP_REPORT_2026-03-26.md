# AIFR3D Plugin Shipping Gap Report

Date: 2026-03-26
Repository: `/home/north3rnlight3r/Projects/AifredVst-2.2.4-beta`

## Purpose

This document explains, in detail:

- what has already been built
- what is partially done
- what still must be done to make the plugin professionally shippable
- what must be fixed before packaging
- what must be proven through testing before any honest release claim can be made

This is a release-readiness and gap document.

It is not a changelog and it is not marketing copy.

## Important Truth First

Your target is:

- 0 bugs
- 0 crashes
- 0 CPU bogging
- 0 RAM bogging

That is the correct release goal.

But technically, no serious software team can promise literal zero bugs in advance.

The honest professional standard is:

- eliminate known correctness bugs
- eliminate known crash paths
- eliminate obvious performance spikes and leaks
- prove stability under test
- ship only after a regression matrix passes

So this document is written as:

- what is already strong
- what still breaks trust
- what must be done to get the plugin to a release-grade state that is stable, polished, and professionally packageable

## Executive State

Current state:

- the plugin is no longer an empty shell
- there is real DSP analysis code
- there is a real UI model and live metering path
- there is a real reference model
- there is a real interpretation/fix-list layer
- the halo, fingerprint, candles, stereo view, and panels are wired to live data paths

But:

- the repo still contains architectural duplication
- the canonical DSP path is better than the modular path, but still not finished
- there are known trust-breaking math issues
- compare/reference behavior is partly coherent and partly transitional
- versioning and packaging still say `2.2.4` in many places
- there is not yet enough hard verification to claim "professionally shipped"

## What Has Already Been Done

This section is the positive inventory.

## 1. The plugin has a real analysis pipeline

The main live analysis path exists in:

- `Source/DSP/AnalysisEngine.cpp`
- `Source/DSP/LoudnessEBUR128.cpp`
- `Source/DSP/SpectrumAnalyzerFFT.cpp`
- `Source/DSP/StereoAnalyzer.cpp`
- `Source/DSP/FeatureExtractor.cpp`

What is already present:

- DC-offset removal before measurement
- FFT-based band analysis
- spectral tilt calculation
- stereo field analysis
- correlation and phase-risk-related metrics
- momentary, short-term, and integrated loudness
- true peak using oversampling
- transient-density-related logic
- volatility tracking
- reference-aware classification

This is not a fake visualizer anymore.

There is an instrument core.

## 2. The plugin has a real UI data bridge

The live analyzer does not dump heavy work into paint calls.

Key files:

- `Source/UI/UiModel.cpp`
- `Source/UI/UiModel.h`

What is already done:

- conversion from `AnalysisFrame` into `MixMetrics`
- storage of live measured values for UI use
- smoothing and meter ballistic behavior
- session candle storage
- realtime candle storage
- compare snapshot storage
- live/reference/compare presentation mode state

This is a serious architectural improvement because it separates:

- DSP
- interpretation
- display state
- rendering

## 3. The plugin has a defined three-mode surface

The current UI already distinguishes:

- Analyze
- Compare
- Reference

Files involved:

- `Source/UI/MainView.cpp`
- `Source/UI/TopBar.cpp`
- `Source/UI/MixSignatureMeter.cpp`
- `Source/UI/ApprovalHalo.cpp`

This is important because a professional analyzer needs mode-specific behavior, not one generic surface pretending to do three jobs.

## 4. The halo system is already substantial

The main diagnostic halo exists in:

- `Source/UI/ApprovalHalo.cpp`

What is already done:

- four-segment diagnostic layout
- mode-aware center text
- mode-aware footer text
- live tone/stereo/loudness/dynamics segment logic
- reference-aware color evaluation
- compare-mode delta behavior
- soft spectrum overlay inside/behind the halo
- pulse-mode emphasis handling

That means the core flagship instrument is already structurally there.

## 5. The fingerprint system is already substantial

The Mix DNA / fingerprint surface exists in:

- `Source/UI/MixSignatureMeter.cpp`

What is already done:

- 12-axis fingerprint
- mapping from measured/derived metrics to axes
- reference ghost overlay
- compare overlay behavior
- metric cards
- band rows
- fingerprint guide rings and labels

This is already a data-driven visualization, not just abstract graphics.

## 6. Candlestick and session-memory systems exist

Files involved:

- `Source/UI/UiModel.cpp`
- `Source/UI/CandleStoryline.cpp`
- session store support under modules/core

What is already done:

- session candle buffers
- realtime candle buffers
- session persistence
- last-10-session bounded memory
- per-session deviation fields

That means the history feature is not starting from zero.

## 7. Reference pool architecture exists

Files involved:

- `Source/DSP/ReferenceModel.cpp`
- `Source/DSP/ReferenceModel.h`
- `assets/reference_pools/*`
- `assets/reference_intake/*`

What is already done:

- canonical pool loading
- metric-config loading
- active profile selection
- genre matching
- target profile data
- mean and match storage

This is important because a professional reference-aware analyzer needs a consistent reference contract.

## 8. Interpretation / fix-list engine exists

Files involved:

- `Source/Interpretation/DiagnosticEngine.cpp`
- `Source/Interpretation/AifredReport.h`

What is already done:

- deterministic interpretation of measured values
- issue generation
- fix-list population
- plain-English status strings for the UI

This means the plugin is not only measuring; it is also translating measurements into understandable advice.

## 9. Build/test scaffolding already exists

Files involved:

- `CMakeLists.txt`
- `CMakePresets.json`
- `tests/*`
- `scripts/install_local_linux.sh`
- `scripts/install_windows.ps1`

What is already done:

- multi-target CMake build
- Linux presets
- Windows build path
- automated local install scripts
- several C++ smoke/unit tests

So the project already has the beginnings of a professional release workflow.

## What Is Only Partially Done

These are the areas that exist but are not yet good enough to ship professionally.

## 1. The canonical DSP path is better, but not fully trustworthy yet

The canonical path is the correct direction.

But the repo’s own audit already identified unfinished correctness issues.

Known examples:

- mid-only activity detection risk
- crest factor and dynamic range conceptual collision
- inconsistent normalization between live and reference frequency-balance logic
- some heuristic proxy layers still present in feature extraction

Meaning:

- the architecture is in place
- but the math still needs a hardening pass before final release

## 2. Compare mode is functional, but not yet fully premium

Current compare mode is real and useful.

But it is still a transitional implementation.

What is present:

- Mix A capture
- Mix B capture
- Mix A vs live B fallback
- compare fingerprint overlay
- compare halo delta logic

What is still missing for a flagship forensic A/B mode:

- stronger dedicated compare-only layout identity
- clearer compare-specific readouts
- potentially dual-halo compare implementation if that remains the product direction
- more explicit compare-meter labeling and operator workflow

## 3. Reference mode exists, but still needs trust hardening

Reference mode already works conceptually:

- live measurement stays live
- reference influences interpretation and corridors

But remaining risks include:

- inconsistent band normalization against target data
- reliance on target data integrity without enough corruption-handling UX
- need for more explicit reference-missing and reference-invalid states

## 4. The UI is already serious, but not fully polished

The UI is much more than placeholders now.

But to ship professionally, it still needs:

- typography consistency review
- panel spacing pass
- resizing and scaling verification
- host window edge-case checks
- final mode-distinction polish
- consistent minimum readable text size enforcement across all panels

## 5. Packaging exists, but release-grade packaging is incomplete

Scripts exist for Linux and Windows.

But full professional packaging still needs:

- final version renaming from `2.2.4` to the ship target version
- installer verification
- clean artifact naming
- end-user folder sanity review
- signed or at least verified release bundles where possible
- release notes aligned with actual shipped code

## What Still Must Be Done Before the Plugin Can Honestly Ship

This is the core of the document.

## A. Eliminate architecture conflict

This is the single biggest structural blocker.

The repo still contains two analyzer stacks:

### Canonical path

- `Source/DSP/*`
- `Source/UI/*`
- `Source/Plugin/*`

### Modular path

- `modules/metering/*`
- `modules/ui/*`
- `modules/aifr3d_core/*`
- `apps/dawai/*`

Problem:

- these paths do not implement the same truth
- they do not use the same math
- they do not use the same fallback logic

Release requirement:

- one analyzer authority only

Practical shipping action:

1. declare the canonical measurement authority to be `Source/DSP/*`
2. remove or retire modular approximations as authoritative signal sources
3. if modular UI/app layers remain, make them consume canonical metrics instead of their own separate meter math

Until that is done, the product is not fully coherent.

## B. Fix the known trust-breaking DSP issues

This is the math hardening pass.

### B1. Fix activity detection

Current issue:

- the repo audit identified mid-only activity gating risk

Why this blocks release:

- wide or anti-phase content can be treated as silence
- meters may freeze incorrectly

Required fix:

- base activity detection on combined L/R/Mid/Side or total stereo energy

### B2. Separate crest factor from dynamic range

Current issue:

- dynamic range language and crest-factor language are not cleanly separated

Why this blocks release:

- a professional analyzer cannot mislabel one metric as another

Required fix:

- store and display crest factor and dynamic range as separate concepts
- audit all UI labels and fix-list language that currently blur the two

### B3. Standardize live/reference band normalization

Current issue:

- live and reference frequency-balance math are not guaranteed to be in the same domain

Why this blocks release:

- corridor comparisons can become misleading

Required fix:

- choose one consistent energy domain and use it in:
  - live analysis
  - serialized reference profiles
  - scoring
  - corridor comparison
  - fingerprint target overlays

### B4. Rework proxy-heavy feature layers

Current issue:

- parts of `FeatureExtractor.cpp` still convert engineering measurements into heuristic proxy values too early

Why this blocks release:

- the analyzer should be built from primary measurements first, interpretation second

Required fix:

- keep:
  - RMS
  - peak
  - crest
  - transient-event timing
  - correlation
  - side/mid energy
- move softer interpretations like mud/harshness/air farther up into interpretation layers if needed

### B5. Validate loudness agreement

Current state:

- the loudness engine is the strongest part of the stack
- but agreement against professional reference tools has not been proven tightly enough

Release requirement:

- build a calibration harness against reference analyzers
- compare:
  - integrated LUFS
  - short-term LUFS
  - momentary LUFS
  - true peak

Until calibration tests exist, "professional-grade loudness agreement" is still a claim, not a proof.

## C. Rebuild the candle system strictly in real units

Current state:

- the plugin-side candle path is stronger than older proxy-based designs
- but candle semantics still need a final forensic check

Release requirement:

- realtime candles must be real loudness OHLC windows
- session candles must be derived from actual session metrics, not score-like proxies

Required verification:

- confirm candle open/close/high/low values are traceable back to measured loudness data
- confirm no visual candle body is driven by alignment score or cosmetic normalization pretending to be loudness

## D. Replace generic smoothing with instrument-specific ballistics

Current state:

- the UI already uses smoothing
- but not all meters are using the correct physical/engineering style behavior

Release requirement:

- peaks:
  - instant attack
  - short hold
  - controlled release
- loudness:
  - mostly trust the real analysis window
- spectrum:
  - overlap + power smoothing
- width/correlation:
  - short EMA, not decorative lag

Why this matters:

- bad smoothing makes real analyzers feel fake
- over-smoothed analyzers hide problems
- under-smoothed analyzers look broken and stressful

## E. Finish performance hardening

This is essential for your "no CPU/RAM bogging" target.

## E1. Remove all unnecessary allocations from hot paths

Check and harden:

- audio callback path
- analyzer processing loop
- FFT input/output buffers
- any repeated temporary vectors
- any UI update path running every frame

Professional target:

- no avoidable dynamic allocation in realtime hot paths

## E2. Verify no blocking work on the audio thread

According to `docs/threading_model.md`, the intended model is:

- audio thread: meter tap/write only
- analysis worker: FFT/profile/scoring
- advisory worker: LLM/rule interpretation
- UI thread: rendering

Release requirement:

- verify the actual code still matches that contract
- audit any accidental direct heavy analysis inside the audio callback
- verify no filesystem, network, or lock-heavy work can hit the audio thread

## E3. Retire redundant FFT or duplicate analyzer calls

Release requirement:

- confirm FFT work is done once per required window
- confirm no duplicate spectral work is done for separate panels if one measurement can feed all of them

## E4. Run CPU profiling under realistic workloads

Must test:

- silence
- simple mono voice
- dense stereo music
- loud transient-heavy material
- reference mode active
- compare mode active
- host resize during playback

Without this, "no CPU bogging" is not verified.

## E5. Run RAM/heap stability tests

Must prove:

- no unbounded vector growth
- no session-memory leaks
- no chat/history leak inside plugin session
- no candle persistence growth beyond designed limits

## F. Eliminate crash paths

You asked for zero crashes.

The only serious path toward that is an explicit crash-hunting pass.

## F1. Host compatibility crash sweep

Test in at least:

- FL Studio
- Reaper
- Ableton Live
- Bitwig
- maybe one additional strict VST3 host

Test actions:

- insert plugin
- remove plugin
- bypass/unbypass
- change buffer size
- change sample rate
- stop/start transport
- automate parameters
- reopen saved project
- duplicate plugin instances
- rapid UI open/close

## F2. Defensive null/invalid-state handling

Audit all paths for:

- missing reference pool
- invalid target profile
- empty audio buffer
- zero-length windows
- first-frame warmup
- compare snapshot missing A or B
- bad persisted session data

Release requirement:

- all these states must degrade gracefully
- none of them may crash the plugin

## F3. Persistence corruption handling

Current systems that need corruption safety:

- session candle store
- reference pool files
- metric catalog
- any future plugin settings persistence

Release requirement:

- malformed files must not crash the plugin
- they must fall back cleanly with visible status

## G. Finish professional UI polish

This is not "make it pretty."

This is "make it release-grade."

## G1. Mode distinction

Current mode switching exists.

But shipping polish requires:

- Analyze mode to feel like a live instrument
- Compare mode to feel like a forensic A/B lab
- Reference mode to feel like a target-alignment workspace

If these modes still feel too similar, the user experience will feel unfinished.

## G2. Typography and spacing pass

Need a final pass for:

- no text below readable size
- consistent caption scale
- consistent panel padding
- no crowded labels in the halo center
- no confusing overlap when the host window is small

## G3. Resize and DPI verification

Must verify:

- 1920x1080 default layout
- smaller laptop sizes
- high DPI / scaling scenarios
- plugin host scaling on Windows and Linux

## G4. Plain-English operator language cleanup

Your product requirement is explicit:

- technical phrasing should be translated into plain English

That means every visible message should be audited for:

- clarity
- consistency
- no robotic filler
- no contradictory terminology

## H. Finish packaging and release engineering

Current scripts are useful, but final release packaging is not done.

## H1. Version cleanup

This repo still says `2.2.4` in many places.

Before shipping:

- update root CMake project version
- update Android version
- update installer labels
- update UI/tutorial/splash text
- update website product strings
- update worker product strings
- update docs

If this is not cleaned up, the shipped product will look unfinished immediately.

## H2. Artifact naming

Need final decisions for:

- VST3 file names
- standalone file names
- variant naming policy
- whether A/B/C/D variants remain user-visible or are internal build variants only

Professional release rule:

- end users should not see confusing engineering variant names unless there is a real product reason

## H3. Installer verification

For Linux and Windows:

- install
- update over existing install
- uninstall
- reinstall
- verify VST path discovery
- verify standalone launcher paths

## H4. Release bundle verification

Every shipped bundle needs:

- correct assets
- correct splash
- correct icons
- correct docs
- correct download links
- correct version labels

## I. Finish QA and proof of release-readiness

This is the final gate.

## I1. Expand automated test coverage

Needed areas:

- DSP correctness
- reference comparison correctness
- candle correctness
- fingerprint axis correctness
- UI state transitions
- compare snapshot behavior
- corrupted-file fallback handling
- host lifecycle behavior where possible

## I2. Build a golden calibration set

Needed to validate:

- loudness
- true peak
- spectrum band grouping
- correlation
- width
- phase-risk behavior

This should be done with fixed audio files and expected outputs.

## I3. Run endurance tests

Needed scenarios:

- long playback session
- repeated open/close cycles
- repeated mode switching
- repeated compare capture
- repeated project load/save in host

Why:

- many crashes and leaks only show up over time

## I4. Build a release checklist and enforce it

Before shipping, someone should be able to answer yes to all of these:

- Does the plugin build cleanly on supported platforms?
- Does it load in supported hosts?
- Does it survive sample-rate and buffer-size changes?
- Does it stay responsive during playback?
- Do all major meters reflect real measurement paths?
- Does reference mode avoid fake targets?
- Does compare mode avoid fake motion?
- Do candles use real units?
- Do no known crashers remain?
- Are version strings coherent?
- Are installers verified?

If any answer is no, it is not ready.

## What Has To Be Removed Or Disabled Before Shipping

If you want a professionally trustworthy shipping build, these things should not remain ambiguous.

## 1. Retire non-canonical analyzer authority

The modular approximate meter path must not remain an equal truth source.

## 2. Remove fake or invented reference fallback behavior anywhere it still exists

If no valid reference exists:

- show live-only mode
- show neutral evaluation
- say reference is unavailable

Do not invent a fake target.

## 3. Remove or isolate preset/action behavior if it conflicts with product truth

Your stated product direction says the analyzer should not silently manipulate sessions or invent fixes as DAW actions.

Any such path should be:

- removed
- or explicitly isolated as optional tooling, not core analyzer behavior

## What a Shippable Plugin Should Look Like At The End

At the end of this work, the plugin should behave like this:

- it loads cleanly in supported hosts
- it does not freeze on silence, stereo width extremes, or missing reference data
- all meters come from one canonical measurement engine
- the halo is fast, truthful, and easy to read
- the fingerprint is responsive but not noisy
- compare mode is a clear technical A/B surface
- reference mode never lies about the live value
- candles are real
- CPU stays controlled under normal music workloads
- RAM stays bounded over long sessions
- all user-facing language is plain and consistent
- installers and docs match the actual shipped version

## Recommended Work Order To Reach Shipping State

If you want the shortest route to a professional release, do the remaining work in this order.

### Phase 1. Lock the canonical measurement path

- choose `Source/DSP/*` as the authority
- remove or demote duplicate analyzer stacks

### Phase 2. Fix trust-breaking DSP issues

- activity detection
- crest vs dynamic range separation
- reference normalization
- proxy cleanup

### Phase 3. Rebuild/verify candles and meter ballistics

- real-unit candles
- proper ballistic behavior

### Phase 4. Performance and crash hardening

- allocation audit
- thread audit
- profiling
- host compatibility sweep

### Phase 5. Final UI polish

- mode distinction
- spacing
- typography
- resize/DPI testing

### Phase 6. Versioning and packaging

- rename to the intended ship version
- verify installers
- verify artifacts

### Phase 7. Release validation

- automated tests
- golden reference tests
- endurance runs
- release checklist

## Bottom Line

What has been done:

- the plugin is no longer a concept
- there is a real analyzer
- there is a real UI model
- there are real modes
- there is a real halo
- there is a real fingerprint
- there is real reference architecture
- there is real build scaffolding

What still needs to be done:

- remove architectural duplication
- harden the DSP math
- eliminate known trust breaks
- tighten meter ballistics
- verify candle truth
- crush performance risks
- run host/crash QA
- complete packaging/version cleanup
- prove stability before release

That is the difference between "working build" and "professionally shippable product."
