# System Map

## Runtime Layers

1. Audio Thread (`audio_engine`)
2. Analysis Worker (`metering` + `reference_engine` + `aifr3d_core`)
3. Advisory Worker (`advisory_layer`, never on audio thread)
4. UI Thread (`ui` + app shell)

## Core Data Flow

1. Audio callback gathers transport/track output.
2. Meter taps emit deterministic metrics.
3. Reference engine computes deltas and pool comparisons.
4. AIFR3D core scores deviations and generates fix list + reports.
5. Advisory layer rewrites guidance into user-facing options.
6. UI presents TRUE / COMPARE / REFERENCE modes and non-destructive apply actions.

## Dependency Rules

- `metering` has no project-module dependencies.
- `reference_engine` depends only on `metering`.
- `aifr3d_core` depends on `metering` + `reference_engine`.
- `advisory_layer` depends on `aifr3d_core` outputs only.
- `audio_engine` depends on metering/reference interfaces but not advisory.
- `ui` depends on module interfaces only.
