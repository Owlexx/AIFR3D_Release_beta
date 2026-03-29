# GUI Map AIFRED A (v2.2.4 Beta a)

## Layout
- Left rail: `Mix Signature` + `Session Candlesticks`.
- Right rail: `Mix Alignment Halo` + `Stereo Image` + `Fix List` + `Insight Chat`.

## Meter Map (Beginner-Friendly)

### Mix Signature
- Signals: long-horizon identity of your mix (not an instant peak meter).
- Uses: LUFS, true peak, crest factor, stereo width/correlation, transient density, and tonal bands.
- How it changes: slowly, because it is averaged and smoothed over time.
- How to read it:
- `Your Mix Signature` bar: where your current session sits.
- `Reference Signature` bar: nearest-track live reference with genre-mean target support.
- `Delta vs reference`: positive means your session signature is pushing above the matched reference.

### Mix Alignment Halo
- Signals: how close your current mix is to the live reference pool and strict genre target window.
- Uses deterministic DSP segments: Tone, Dynamics, Space, Punch, Balance.
- How it changes: updates from live analysis frames while staying stable enough to interpret.
- How to read it:
- Center `%`: overall alignment score.
- Target rule: the visible target is `±3` around the detected genre mean, but nearest-track deviation still affects the score.
- Segment arcs: which area is weak right now.
- Live diagnostic line: current issue inferred from flags and live metrics (clip risk, phase risk, harshness, transient softening, width collapse).

### Stereo Image
- Signals: stereo field health and mono compatibility.
- Uses: stereo width, mid energy (center focus), correlation.
- How to read it:
- `Width`: larger side energy spread.
- `Correlation`: near `+1` stable mono-compatible, near `0/-1` phase risk.

### Session Candlesticks (Rolling 10 Sessions)
- Signals: session-to-session signature behavior, not per-second animation.
- Candle = one completed session snapshot.
- Window = last 10 sessions (ring buffer).
- How to read it:
- `Open/Close`: start/end session signature.
- `High/Low`: session range volatility.
- Footer deltas: session deviation metrics (LUFS, crest, width, transient) vs prior session.

### Fix List
- Signals: actionable priorities grounded in current DSP flags + advisory context.
- Behavior: entries stay visible (no flashing churn); scroll wheel enabled.
- How to read it:
- `HIGH/MED` badge = impact.
- `Signal reliability` = confidence weight.

### Insight Chat
- Signals: contextual guidance linked to analysis outputs.
- Layout: scrollable chat log + prompt input stacked (no overlay over fix list).
- Behavior: async guidance; no blocking audio thread.

## Tab/Mode Policy
- VST exposes `Analyze`, `Compare`, `Reference` modes only.
- No `Studio` tab in VST.
