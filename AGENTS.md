AIFR3D MASTER ENGINEERING PROMPT


You are implementing the core DSP analysis engine for AIFR3D.
Do not approximate metrics.
Do not invent simplified proxies.
Implement the following metrics using standard audio engineering formulas.
All analysis must run in the analysis layer, never inside GUI paint loops.
Audio input format:


float bufferL[n]
float bufferR[n]
sampleRate
bufferSize
PREPROCESSING
Before analysis:
Remove DC offset.
Compute:


xL = bufferL - mean(bufferL)
xR = bufferR - mean(bufferR)
Normalize if required for internal calculations.
MID / SIDE TRANSFORM
Compute Mid and Side channels.


Mid  = (L + R) * 0.5
Side = (L - R) * 0.5
Store Mid and Side buffers.
These are required for stereo, spatial, and correlation metrics.
TRUE PEAK (dBTP)
True peak must detect inter-sample peaks.
Oversample the signal (4x recommended).
Example approach:


upsampled = polyphase_oversample(signal, 4x)
peak = max(abs(upsampled))
truePeak_dBTP = 20 * log10(peak)
Use max across L and R.
PEAK dBFS
Simple sample peak.


peak = max(abs(sample))
peak_dBFS = 20 * log10(peak)
RMS
Window RMS.


RMS = sqrt(mean(x^2))
RMS_dB = 20 * log10(RMS)
Compute separately for L, R, Mid, Side if needed.
CREST FACTOR
Crest Factor measures transient punch.
Copy code

CrestFactor = Peak / RMS
Crest_dB = 20 * log10(Peak / RMS)
High crest = dynamic
Low crest = compressed
LUFS (ITU-R BS.1770)
Use standard LUFS measurement.
Apply K-weighting filter.
Filter chain:
High-shelf filter + high-pass filter.
Compute gated loudness.
Steps:
Apply K-weighting filter
Compute momentary loudness
Apply gating threshold
Integrate over time
Integrated LUFS formula:


LUFS = -0.691 + 10 * log10(sum(weighted_energy))
Implement:
Integrated LUFS
Short-term LUFS (~3 sec window)
Momentary LUFS (~400 ms window)
TRANSIENT DENSITY
Measure transient events per second.
Detect transients using:


energyFlux = sum((x[n] - x[n-1])^2)
Threshold detection.
Count transient events over time window.


transientDensity = transients / seconds
High density = busy mix
Low density = sparse mix
STEREO CORRELATION
Measure correlation between L and R.


corr = sum(L * R) / sqrt(sum(L^2) * sum(R^2))
Range:


+1 = mono
0  = wide
<0 = phase conflict
STEREO WIDTH
Derived from Mid and Side energy.
Copy code

midEnergy  = mean(Mid^2)
sideEnergy = mean(Side^2)

width = sideEnergy / (midEnergy + sideEnergy)
Normalized 0–1.
0 = mono
1 = extremely wide
PHASE RISK
Detect negative correlation regions.
Copy code

phaseRisk = max(0, -correlation)
Additional check for low-frequency phase risk using bandpass filtering.
FFT SPECTRUM ANALYSIS
Use FFT size:
Copy code

2048 or 4096 samples
Apply window function:
Copy code

Hann window
Compute magnitude spectrum:
Copy code

FFT -> complex bins
mag = sqrt(real^2 + imag^2)
dB = 20 * log10(mag)
Frequency mapping:


freq = binIndex * sampleRate / FFTsize
BAND ENERGY GROUPING
Group FFT bins into frequency bands.
Example ranges:
Sub: 20–60 Hz
Low: 60–120 Hz
LowMid: 120–400 Hz
Mid: 400–2000 Hz
HighMid: 2k–6k Hz
High: 6k–12k Hz
Air: 12k–20k Hz
Energy calculation:


bandEnergy = mean(magnitudeBins)
band_dB = 20 * log10(bandEnergy)
SPECTRAL TILT
Measure overall brightness slope.
Compute linear regression on log-frequency magnitude.


tilt = slope(dB vs log(freq))
Positive slope = bright
Negative slope = dark
MULTIBAND DELTA (REFERENCE COMPARISON)
When reference data exists:


delta = liveBand_dB - referenceBand_dB
Used for tonal bars.
SPATIAL FIELD (WXYZ)
Compute spatial axes.


W = mean(Mid^2)
X = mean(L^2)
Y = mean(R^2)
Z = mean(Side^2)
Normalize values.
These define spatial distribution.
CANDLESTICK DATA (LOUDNESS MARKET)
Realtime candle window example:


window = 1 second
Compute:


open  = LUFS at window start
close = LUFS at window end
high  = max(shortTermLUFS)
low   = min(shortTermLUFS)
Store candle.
Session candles store same metrics across sessions.
VOLATILITY INDEX
Measure signal instability.
Compute weighted variance of:


LUFS change
crest change
spectral change
stereo width change
correlation change
frequency change
peak_dBFS change
tonal balance change
Eq change

Example:


volatility =
  w1 * variance(LUFS)
+ w2 * variance(Crest)
+ w3 * variance(SpectralTilt)
+ w4 * variance(Width)
+ w5 * variance(Correlation)
Normalize to index scale.
MIX DNA AXES
Derived from normalized metrics.
Example axes:


SubWeight = subBandEnergy
LowMidDensity = lowMidEnergy
Brightness = spectralTilt
Dynamics = crestFactor
TransientImpact = transientDensity
StereoWidth = width
CenterStrength = midEnergy
PhaseStability = correlation
LoudnessPressure = integratedLUFS
Normalize 0–1.
Used for fingerprint visualization.
SMOOTHING
Use exponential smoothing.


smooth = alpha * new + (1-alpha) * old
Different alpha values:
Fast meters → alpha ≈ 0.6
Derived metrics → alpha ≈ 0.3
Long horizon metrics → alpha ≈ 0.1
PERFORMANCE RULES
Do NOT:
• allocate memory in the paint loop
• run FFT in GUI thread
•
All analysis must run based on REALTIME BUFFER vs REFERENCE 
GUI may  render cached panel layout and text displays but must render all meters based on the REALTIME BUFFER and display real metrics and results.
DO NOT DISPLAY CACHED METRIC Displays
Fix List Display: responses are generated from Cached metric information and compares BUFFER vs REFERENCE

FINAL REQUIREMENT
The analyzer must behave like a real engineering instrument.
All meters must represent true signal measurements derived from correct DSP math in correlation with REALTIME BUFFER 
Do not substitute simplified estimations.
All meters must be instantly readable and understandable.
Text inside panels should be no smaller than textSize 14 to ensure a clear view..
Metrics should not display information that is too advanced for begginners. 
Technical phrasing should be interpreted into plain everyday english. Example meter shows tonal imbalance, meter should display "Tonal Balance = Dark/Bright Muddy/Boomy Thin/Brittle etc. 





GUI, Metering, Candlestick System, Volatility, and Mode Architecture

You are acting as the lead systems engineer and visualization architect responsible for finalizing the AIFR3D analyzer plugin.
This is production-level refactoring and product finishing, not experimentation.
Your objective is to transform the current system into a coherent, premium mix-analysis instrument.
The plugin must feel:
• fast
• technically honest
• visually intelligent
• premium quality
• trustworthy
• defined in own category from any existing plugins.

The GUI must behave like a real measurement instrument, not a decorative visualization.
CORE ARCHITECTURE RULE
Separate the metering system into three independent metering modes in correlation to the current function modes::
Analyze Layer
Compare Layer
Reference Layer
These layers must never block one another.
1 — Analyze Layer: 
The meter display and the dsp measurement layer reads only the realtime audio buffer based on audio input.
Meters in Analyze Mode never depend on:
• reference pools
• AI availability
• scoring logic
• network access
Meters must always function as pure instruments.
Measured metrics must include:
Integrated LUFS
Short-term LUFS
True Peak
Crest Factor
Transient Density
Stereo Width
Phase Correlation
Mid Energy
Side Energy
Multiband Energy
Spectral Tilt
WXYZ Spatial Coordinates
Meters must never freeze or stall because of missing reference data.
2 Compare Mode (REFERENCE CONTEXT, A/B COMPARISON)
Reference pools provide context only and serve as an accepted "Range" to base Fix List feedback on.
They must never change measured values.
Reference pools define:
• target corridors
• deviation thresholds
• color interpretation
Example:
Live LUFS = -11
Reference corridor = -9 to -8
Meter still shows -11, but color indicates deviation.
Color states:
Neon Dark Blue = under target
Neon Green = within target
Dark Green = approaching limit
Red = outside safe range
If reference pool data is unavailable:
Meters remain active
Evaluation color on DIAGNOSTIC MIX-HALO defaults to RGB Pulse display that is color coded by frequencies detected and acts as a signal visualizer that reads the realtime buffer and reacts to transient energy. If reference pool data is missing, Halo becomes a live frequency analyzer that detects sound frequencies from 10hz - 20khz and displays an EQ display that explains the color codes and freequencies being displayed.

ALL MEASURED METRICS MUST REMAIN TRUE
3 — AIFR3D INTELLIGENCE LAYER
The AIFR3D brain interprets measurements and generates:
• scoring
• mix diagnostics
• fix lists
• contextual advice
This system reads:
Live measurements
Reference pool models
Session history
Compare track metrics
But never overrides measurement data.
MODE ARCHITECTURE
Analyze, Compare, and Reference must behave like three different instruments.
They must not look like the same interface with minor tweaks.
Switching modes must immediately change:
• layout
• meter style
• workflow emphasis
Users must recognize the mode instantly.
ANALYZE MODE — LIVE DIAGNOSTIC COCKPIT
Purpose: analyze the user's mix in realtime.
Visual style:
• instrument-panel layout
• maximum live meter responsiveness
• minimal reference overlays
Layout structure:
Center
Halo diagnostic instrument
Left
Spatial stereo analyzer
Right
Live tonal band meters
Bottom
Dynamics metrics (LUFS, crest, transient density)
Meters react directly to the realtime buffer.
This mode should feel like flying an aircraft instrument panel for audio.
COMPARE MODE — FORENSIC MIX COMPARISON
Purpose: compare two mixes directly.
Layout:
Left side
Mix A analysis
Right side
Mix B analysis
Center
Difference / delta analysis
Include analysis isolation knobs:
Low band
Low-mid band
Mid band
High band
These knobs must filter the signal before analysis, allowing users to isolate the cause of mix differences.
Compare mode should feel like a forensic audio lab.
REFERENCE MODE — TARGET ALIGNMENT WORKSPACE
Purpose: compare mix against reference pool.
Visual emphasis:
• reference corridors
• deviation meters
• genre profile context
Layout:
Top
Reference profile / genre
Center
Halo instrument with reference overlay
Left
Live tonal meters
Right
Target corridor meters
Bottom
Deviation metrics and AIFR3D advice
This mode must feel like a mastering analysis environment.
HALO SYSTEM (FLAGSHIP INSTRUMENT)
The Halo is the centerpiece.
Halo Segmented Ring
Displays four live metrics:
Tone
Stereo
Loudness
Dynamics
Each quadrant must move independently.
No lazy averaging.
Movement must reflect real signal behavior.
Halo Inner Intersectrum (Spectrum)
The spectrum line inside the Halo must behave like a premium realtime EQ analyzer.
Requirements:
• driven by realtime FFT
• responsive to waveform changes
• smooth but fast motion
Users must clearly see:
• bass movement
• mid buildup
• high frequency energy
Stereo Glow Behavior
The spectrum must glow based on side-channel energy.
More side energy → stronger glow
Mono signals → minimal glow
This communicates tonal shape and stereo behavior simultaneously.
SPATIAL ANALYZER
Implement a full WXYZ spatial coordinate system.
W = mid energy
X = left energy
Y = right energy
Z = side energy
The display must show:
• mono collapse
• balanced stereo
• excessive width
• phase risk
• center dominance
Movement must correspond to real signal analysis.
TONAL BAND METERS
Bands must include:
Sub
Low
Low-mid
Mid
High-mid
High
Each band reacts to real energy.
Color indicates reference deviation.
Meters remain functional without reference pool data.
AUDIO CANDLESTICK SYSTEM
Implement two candlestick modes.
Candles must behave like financial trading charts.
MODE 1 — SESSION HISTORY
Stores last 10 mix sessions.
Each candle represents one session.
Open = starting integrated LUFS
Close = ending integrated LUFS
High = highest short-term LUFS
Low = lowest short-term LUFS
Candle body = loudness change across session.
Wicks represent dynamic peaks and dips.
Color logic:
Green = improvement
Red = regression
Gold = plateau
The chart must support high sensitivity, allowing small loudness improvements to remain visible.
MODE 2 — REALTIME BUFFER CANDLES
Each candle represents a short realtime analysis window.
Example window:
0.5–1 second
Open = LUFS at start
Close = LUFS at end
High = peak loudness
Low = quietest moment
Tall candles indicate strong loudness movement.
Small candles indicate stable audio.
Wicks represent transient spikes.
This must visually resemble professional trading software.
AUDIO VOLATILITY INDICATOR
Implement an audio volatility index similar to financial market volatility.
Purpose:
Measure signal instability and dynamic behavior.
Volatility must derive from:
LUFS variation
Crest factor variation
Transient density changes
Peak-to-average fluctuation
Stereo width variation
Correlation fluctuation
Spectral tilt changes
Multiband energy movement
REALTIME VOLATILITY
Shows signal stability in realtime.
Interpretation:
Low volatility
= compressed / stable
Medium volatility
= controlled dynamic movement
High volatility
= unstable / erratic signal
SESSION VOLATILITY
One volatility reading per mix session.
This shows whether mixes are becoming:
• more controlled
• more dynamic
• more unstable
Display as:
• line graph
• volatility index
• histogram
PERFORMANCE REQUIREMENTS
Ensure smooth operation.
Investigate and remove:
• memory leaks
• repaint thrashing
• redundant FFT calls
• repeated allocations
• heavy paint logic
Target performance:
Stable 60 FPS UI responsiveness.
GUI POLISH REQUIREMENTS
Improve readability across the interface.
Requirements:
Larger text
Better spacing
Clear panel hierarchy
Less visual clutter
Default layout must look correct at 1920×1080.
Panels should be resizable where practical.
FINAL PRODUCT GOAL
The finished plugin must feel like:
A professional audio measurement instrument
combined with
an intelligent mix analysis system.
Users must trust the meters immediately.
The interface must visually communicate that it is reading real audio behavior.
FINAL DELIVERABLE
When finished provide:
Files modified
Systems refactored
Meters corrected
Math validated
Performance fixes
Mode visual differences
Candlestick implementation
Volatility system implementation
FINAL RULE
Do not leave:
• fake analyzer motion
• duplicated DSP paths
• placeholder math
• decorative visuals with no signal meaning
Everything must be real, coherent, and production-ready.

You are implementing a new flagship visualization system for AIFR3D called Mix DNA Fingerprint.
Its purpose is to show the overall sonic identity of a mix using a real multi-axis fingerprint derived from measured and derived analysis metrics.
This is not decorative. It must come from real signal behavior.
Create a radial or multi-axis fingerprint visualization that represents the mix across dimensions such as:
Sub Weight
Low-End Control
Low-Mid Density
Mid Presence
High-Mid Bite
Air / Brightness
Dynamics
Transient Impact
Stereo Width
Center Strength
Phase Stability
Loudness Pressure
Requirements:
derive the fingerprint from real measured and derived metrics
do not feed it from arbitrary scoring-only values
smooth it enough to be readable but keep it responsive
support different behavior by mode:
Analyze mode: live mix fingerprint only
Compare mode: Mix A vs Mix B fingerprint
Reference mode: live mix vs reference pool fingerprint
make the live fingerprint visually distinct from the target/reference fingerprint
if reference mode is active, allow target corridor or ghost overlay
ensure the visualization feels premium, legible, and data-driven
let AIFR3D use the fingerprint as part of contextual explanations and mix advice
do not create fake movement or abstract art; every axis must map to a defined metric family
keep the implementation performant and outside the paint loop for heavy calculations
Final goal: Users should instantly understand the overall sonic identity of their mix and how it differs from a compare source or reference target.
