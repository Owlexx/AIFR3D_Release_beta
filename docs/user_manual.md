# AIFR3D 2.2.4 Beta User Manual

## Product Surfaces

- VST3 plugin
- standalone app
- website store and catalog
- Android admin app

## Core Behavior

- AIFR3D reads live audio from the realtime buffer.
- Scores, halo state, mix signature, and fix-list output react to live DSP changes.
- Extreme EQ changes must move spectral deltas, issue ranking, and alignment score.
- The fix list persists during the session until a fresh analysis pass replaces it or the user clears it.
- Chat history persists in a scrollable session view.

## Core DSP Metrics

- Spectral balance
- Tonal tilt
- Loudness structure
- Dynamic range
- Transient density
- Stereo width and correlation
- Low-end mono integrity

## Frequency Bands

- Sub `20-60 Hz`
- Low `60-200 Hz`
- Low-Mid `200-800 Hz`
- Mid `800 Hz-2 kHz`
- High-Mid `2-6 kHz`
- High `6-16 kHz`

## Reference Contract

- Genre target = detected genre mean
- Target window = `±3`
- Mix deviation is still measured against the nearest individual reference track
- Metrics are not flattened to a single genre-wide mean value

## Chat Contract

- OpenAI only
- exact key path first: `/home/north3rnlight3r/Documents/API_KEY_INDEX/OpenAI API Key.txt`
- no local fallback
- no silent downgrade
- missing key message: `OpenAI API key missing — AIFR3D chat unavailable`

## Purchase Contract

- VST3 Plugin: `$29.99`
- Standalone App: `$49.99`
- Beats: `$19.99`

## Website Release Material

- installation instructions
- 2.2.4 Beta README
- minimum PC requirements
- release log
- VST guide
- standalone guide

## Support

`north3rnlight3rofficial@outlook.com`
