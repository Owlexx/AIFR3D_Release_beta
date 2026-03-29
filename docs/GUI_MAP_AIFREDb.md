# GUI Map AIFRED B (v2.2.4 Beta b)

## Layout
- Top diagnostics strip: `Mix Signature`, `Mix Alignment Halo`, `Stereo Image`.
- Bottom split: `Session Candlesticks` (left), `Fix List + Insight Chat` (right).

## Meter Interpretation
- Meter behavior and DSP mapping are identical to Variant A.
- Only placement and visual flow differ.

### Fast Read Order
1. `Mix Alignment Halo` center score and weakest segment.
2. `Mix Signature` delta vs nearest reference + genre target.
3. `Stereo Image` correlation warning.
4. `Fix List` top `HIGH` items.
5. `Session Candles` trend across last 10 sessions.

## Beginner Labels
- Loudness: `LUFS`
- True Peak: `dBTP`
- Punch: `Crest factor (dB)`
- Stereo: `Width` and `Correlation`
- Tonal behavior: `Tone / Mud / Harsh / Air` (via segment diagnostics)
- Spectrum graph: `20 Hz -> 20 kHz`, `-32 dB -> +3 dB`, redline at `>= 0 dB`

## Mode Policy
- VST modes: `Analyze`, `Compare`, `Reference`.
- No studio mixer tab in VST.
