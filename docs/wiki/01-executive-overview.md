# AIFR3D / DawAI Executive Overview

## Product Thesis
AIFR3D is built around a strict separation of concerns:

1. Deterministic DSP truth (measurable, repeatable, testable)
2. Probabilistic guidance (advisory language, never DSP authority)

That separation enables trust: numbers stay stable, while guidance stays flexible.

## Ecosystem Surfaces
The same core contracts are shared across all surfaces:

- VST3 plugin (`Source/`)
- Standalone analyzer app (`Source/` standalone target)
- Desktop admin app (`apps/dawai`)
- Android admin app (`apps/dawai_admin_android`)

Shared source-of-truth artifacts:

- `assets/aifr3d_brain/brain_config.json`
- `docs/dsp_math_pack_v1.md`
- `docs/schema/report.schema.json`
- `modules/aifr3d_core/include/dawai/aifr3d_core/system_contract.hpp`

## Architecture Properties

### 1) Deterministic core
Most "AI audio" products collapse scoring, UI, and language in one opaque path. DawAI separates them:

- `metering` computes metrics deterministically
- `reference_engine` computes benchmark deltas
- `aifr3d_core` computes scores/fix lists
- `advisory_layer` can only reinterpret outputs, not rewrite DSP reality

### 2) Cross-surface reuse
One metric model can power:

- DAW plugin feedback
- mobile admin diagnostics
- desktop batch analysis/reporting

This reduces drift between product surfaces and simplifies maintenance.

### 3) Deployment flexibility
The system works in local-first mode:

- rule-based adviser works offline
- local Ollama adapters can be enabled
- no paid API key is required for baseline operation

## Delivery Signals

- Realtime-safe threading model documented and enforced (`docs/threading_model.md`)
- CI builds for Linux, Arch/Garuda, Windows, and Android
- deterministic golden tests for scoring/reporting behavior
- schema-driven report format for downstream integrations

## Near-Term Engineering Path

1. Increase reference pool depth by genre (already structured via profile caches)
2. Expand pool-level robust stats (percentiles, confidence bands)
3. Expose report exports and trend dashboards to team workflows
4. Keep advisory models pluggable while preserving deterministic truth authority
