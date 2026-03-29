# AIFRED DSP Math Pack (V1)

Canonical deterministic metric definitions for scoring and reference matching.

## Shared Definitions
- Mid: `M[n] = (L[n] + R[n]) / 2`
- Side: `S[n] = (L[n] - R[n]) / 2`
- STFT: Hann, `N=2048`, `H=512`
- Band energy: `E_B(t) = sum_{k in B} |X_t[k]|^2`
- Log band energy: `e_B(t) = 10*log10(E_B(t)+eps)`
- Logistic squash: `sigma(x) = 1/(1+exp(-x))`
- Robust z-score: `(x - median(pool)) / (1.4826*MAD(pool)+eps)`

## Harshness Index (0..1)
Bands: harsh `2.2k-5.5k`, control `500-2k`, air `10k-16k`.
- `r_t = e_H(t) - e_C(t)`
- `b_t = max(0, r_t - r_{t-1})`
- Air exemption from `e_A - e_C`
- `harshness = sigma((w1*mean(r)+w2*mean(b)-w3*airExempt - h0)/s_h)`

## Mud Index (0..1)
Bands: mud `180-420`, body `60-160`, presence `1.5k-4k`.
- `r1_t = e_MUD - e_BODY`
- `r2_t = e_MUD - e_PRES`
- add steadiness term from variance of mud energy
- `mud = sigma((0.45*mean(r1)+0.45*mean(r2)+0.10*steadiness - m0)/s_m)`

## Transient Match (0..1)
- Spectral flux envelope from STFT
- Normalize flux with robust stats
- Features: transient density, peak sharpness, optional onset interval stability
- Distance to reference: weighted Euclidean
- Match: `exp(-alpha * distance)`

## Stereo Signature Match (0..1)
- Per-band side ratio: `W_B = E_{S,B} / (E_{M,B}+E_{S,B}+eps)`
- Bands: Sub, Bass, LowMid, Mid, HighMid, Air
- Build signature from per-band mean + std and correlation stability
- Cosine similarity with penalty for negative correlation time

## Segment Scores
- Tone, Dynamics, Space, Punch, Balance from weighted deterministic features
- Life index combines microdynamics, transient match, space, and air
- Confidence from playback duration and stability
- Approval is confidence-gated weighted sum of segment scores

## Event Triggers
- `phase_risk`: sustained negative correlation
- `clip_risk`: true peak above threshold
- `harshness_burst`: rapid harsh-band rise
- `width_collapse`: sudden side-energy drop
- `transient_softening`: crest and transient density drop together
