# DSP Math Deep Dive

This page explains how AIFR3D computes its deterministic metrics in engineering terms.

Primary canonical spec: `docs/dsp_math_pack_v1.md`.

## Signal Model

Stereo input is transformed into Mid/Side:

- `M[n] = (L[n] + R[n]) / 2`
- `S[n] = (L[n] - R[n]) / 2`

Short-time analysis uses Hann-window STFT:

- FFT size `N = 2048`
- hop size `H = 512`

Band-energy primitive:

- `E_B(t) = sum(|X_t[k]|^2)` for bins in band `B`
- `e_B(t) = 10*log10(E_B(t) + eps)`

## Core Tone Metrics

### Harshness Index (0..1)
Measures brittle upper-mid behavior, especially bursty spikes.

Typical bands:

- harsh: `2.2kHz - 5.5kHz`
- control: `500Hz - 2kHz`
- air exemption: `10kHz - 16kHz`

Key idea:

- ratio term: harsh minus control
- burst term: positive frame-to-frame rise
- air exemption reduces false positives for airy but controlled mixes

### Mud Index (0..1)
Measures sustained low-mid congestion.

Typical bands:

- mud: `180Hz - 420Hz`
- body control: `60Hz - 160Hz`
- presence control: `1.5kHz - 4kHz`

Key idea:

- mud is penalized when elevated vs body and presence
- sustained mud (low variance) increases score severity

### Air Index (0..1)
Measures openness in high frequencies while penalizing spiky behavior.

## Dynamics and Impact Metrics

### Crest factor
- `crest_dB = 20*log10(peak / rms)`

### Microdynamic index
Uses robust spread (MAD-derived) of short-window loudness to detect over-flattening.

### Compression collapse risk
Combines:

- depressed crest
- reduced microdynamic spread
- true peak proximity to ceiling

### Transient density
Uses normalized spectral flux threshold crossings as onset proxy.

## Stereo/Space Metrics

### Correlation
Windowed L/R correlation statistics produce:

- mean correlation
- fraction of time below zero (`p_neg`)

### Sub mono integrity
Compares Mid vs Side energy in low band (e.g. 20-120Hz).

### Stereo signature match (vs reference)
Per-band width proportions across musical bands are compared to reference signatures, with penalties for unstable negative-correlation behavior.

## Reference Matching and Genre Logic

In VST runtime (`Source/DSP/AnalysisEngine.cpp` + `ReferenceModel.cpp`):

1. feature frame extracted from live input
2. nearest genre profile selected by distance
3. confidence computed from inverse profile distance
4. active profile stabilized to avoid rapid UI flicker

Genre profile means are loaded from reference profile JSONs when available.

## Scoring Pipeline

Segment outputs:

- Tone
- Dynamics
- Space
- Punch
- Balance

Then:

- `lifeIndex` from movement + transient + space/tone factors
- `confidence` from analysis stability/time logic
- `approval` from weighted segments with confidence gating

## Event Flags and Diagnostics

`AnalysisFrame.flags` sets deterministic event bits:

- `kFlagPhaseRisk`
- `kFlagClipRisk`
- `kFlagHarshnessBurst`
- `kFlagWidthCollapse`
- `kFlagTransientSoftening`

These events feed UI callouts and fix-card prioritization.

## Numerical Stability Practices

- clamp normalized outputs to `[0,1]`
- avoid divide-by-zero with eps constants
- use robust statistics (median + MAD) over naive mean/std where possible
- gate confidence before exposing high approval values

## Engineering Constraint
The advisory layer may explain these metrics, but cannot overwrite them. Deterministic math remains the sole source of truth for scoring.
