# GUI Map AIFRED D (v2.2.4 Beta d)

## Layout
- Stacked bands.
- Top band: `Mix Signature` + `Stereo Image`.
- Middle band: `Session Candlesticks` + `Mix Alignment Halo`.
- Bottom band: `Fix List` + `Insight Chat`.

## Meter Interpretation
- Same deterministic DSP core as A/B/C.
- Same advisory/routing behavior as A/B/C.
- Variant D is optimized for top-to-bottom workflow scanning.

### Top-to-Bottom Meaning
- Top: “What is happening now?” (`Mix Signature`, `Stereo`).
- Middle: “How close am I to target and how did sessions move?” (`Halo`, `Candles`).
- Bottom: “What should I do next?” (`Fix List`, `Chat`).

## Technical-to-Beginner Translation
- `LUFS`: perceived loudness level.
- `dBTP`: true peak safety ceiling.
- `Crest`: punch and transient headroom.
- `Width/Correlation`: stereo spread and mono safety.
- `Mix Signature`: long-horizon identity score relative to the nearest matched reference, while the visible target stays anchored to the genre mean.

## Mode Policy
- VST: Analyze/Compare/Reference only.
- Standalone: includes additional `Studio` tab and 5-channel mixer host.
