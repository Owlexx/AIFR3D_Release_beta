# GUI Map AIFRED C (v2.2.4 Beta c)

## Layout
- Three-column wall.
- Left: `Mix Signature` + `Fix List`.
- Center: `Mix Alignment Halo` + `Session Candlesticks`.
- Right: `Stereo Image` + `Insight Chat`.

## Meter Interpretation
- Same DSP engine/math/routing as A/B/D.
- Deterministic meters are unchanged; only UX arrangement differs.

### What Each Meter Means
- `Mix Signature`: slow identity trend vs nearest reference with genre-mean target context.
- `Mix Alignment Halo`: live quality map with segment-level alignment against the strict `±3` target window.
- `Stereo Image`: width + mono compatibility.
- `Session Candles`: previous-session variation history (10 session memory).
- `Fix List`: stable, scrollable action priorities.
- `Insight Chat`: analysis-grounded assistant narrative.

## Producer-Centric Reading Pattern
1. Check `Halo` for live issue category.
2. Confirm with `Stereo Image` and `Mix Signature` metrics.
3. Apply one `Fix List` action.
4. Re-run and compare latest session candle.
