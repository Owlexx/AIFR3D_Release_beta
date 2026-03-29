# Canonical Build Contract

Authoritative version: **2.2.4**

This repository has one canonical product lineage for active builds:
- Desktop plugin/standalone: JUCE target(s) under `Source/`
- Standalone desktop app: modules under `modules/*` and app entry under `apps/dawai`
- Shared advisory, metering, reference, and session-memory modules

## Canonical Targets

Primary canonical desktop targets:
- `aifred_vst3` (plugin + standalone wrapper)
- `dawai_standalone` (module-driven standalone app)

Variant desktop plugin targets (UI-only differences, identical DSP/routing/advisory):
- `aifred_vst3_A`
- `aifred_vst3_B`
- `aifred_vst3_C`
- `aifred_vst3_D`

## Variant Contract (A/B/C/D)

All A/B/C/D targets must keep identical:
- DSP math and analysis pipeline
- Realtime audio routing and thread model
- Advisory chain and model-routing logic
- Session storage behavior and bounded persistence rules

Only allowed differences between A/B/C/D:
- theme/look-and-feel constants (color palette, gradients, corner radii, typography scale, spacing)

## Version Truth

Version `2.2.4` must be consistent across:
- build metadata (CMake and other package metadata where active)
- plugin/UI runtime version strings
- release-facing docs that describe current active build

Conflicting or obsolete implementation files are not deleted.
They must be moved under `/_legacy/<timestamp>/...` and excluded from canonical build includes.
