# Repository Breakdown and Technical Due Diligence

This page is a technical due-diligence summary for engineering leadership and technical stakeholders.

## 1) Repository Topology

High-level folders:

- `Source/` -> JUCE VST3 + standalone plugin target
- `modules/` -> reusable core modules (audio, metering, reference, scoring, advisory, ui)
- `apps/dawai/` -> desktop app shell/entry
- `apps/dawai_admin_android/` -> Android admin app
- `assets/` -> shared brain config, logos, reference pools
- `docs/` -> architecture, ADRs, schema, manuals
- `scripts/` -> build/install/release/ingestion automation
- `tests/` -> regression + behavior tests

Full file map: `docs/repo_tree_explained.md`

## 2) Module Responsibilities

### `modules/metering`
Deterministic metric extraction primitives (FFT/loudness/stereo).

### `modules/reference_engine`
Reference profile lifecycle, A/B support, pool caching.

### `modules/aifr3d_core`
Deviation scoring, fix-list generation, reporting and apply-fix logic.

### `modules/advisory_layer`
Natural-language advisory bridge (rule-based + local model adapters), with memory store.

### `modules/audio_engine`
Transport, playback, device service, plugin hosting wrappers.

### `modules/ui`
Shared DAW-style UI components and theme system for desktop shell.

## 3) Quality and Governance Signals

### Determinism boundaries
Architecture explicitly isolates deterministic truth from advisory rewrite layers.

### Testing
`tests/` includes:

- transport state tests
- metering signal tests
- reference cache tests
- scoring JSON golden tests
- advisory layer tests
- apply-fix undo tests
- benchmark pool tests

### CI and packaging
GitHub workflows include:

- CI build/test lint
- installer packaging for Linux, Arch/Garuda, Windows, Android
- optional release publishing with artifacts

## 4) Data and Contract Stability

Stable contract artifacts:

- `docs/schema/report.schema.json`
- `docs/dsp_math_pack_v1.md`
- `assets/aifr3d_brain/brain_config.json`
- `system_contract.hpp` constants

Android and desktop/VST remain aligned through `scripts/sync_android_truth_assets.sh`.

## 5) Runtime Risk Controls

- lock-free queue between audio processing and UI consumption
- no advisory operations in the audio callback
- fallback behavior when reference data or adviser is unavailable
- schema-backed report outputs for downstream automation safety

## 6) Delivery Readiness Snapshot

Current strengths:

- clear module ownership
- multi-surface product strategy on one technical core
- deterministic metric contract with test coverage
- platform packaging path (desktop + plugin + mobile admin)

Current engineering focus areas:

- deeper command-surface convergence (`DawAI` binary vs admin templates)
- richer pool statistics and confidence calibration
- host-level validation matrix expansion for plugin distribution
