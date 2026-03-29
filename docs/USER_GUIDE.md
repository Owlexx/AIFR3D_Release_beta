# AIFR3D VST 2.3 & 2.2.5 | User Guide

Welcome to AIFR3D, the first-of-its-kind translation layer between audio physics and human musical perception. This guide will walk you through the operational workflow of the plugin.

---

## 1. Getting Started
To begin using AIFR3D, insert the VST3 plugin on your **Master Bus** or your **Mix Bus**.

### The Version Toggle (Dual-Mode)
At the top right of the plugin, you will find the **V2.2.5 Mode** toggle.
- **Off (2.2.4 Mode)**: Use this for a streamlined, discovery-focused diagnostic view.
- **On (2.2.5 Mode)**: Use this for production-ready precision with session tracking and the full Dawai Advisory Layer.

---

## 2. Reading the Meters
AIFR3D translates complex DSP data into intuitive visual patterns.

### The Diagnostic Halo (Macro Patterns)
The Halo is your fast-read for mix health.
- **Tone (Top)**: Maps spectral balance from **20Hz to 20kHz**. Green indicates a healthy match to your reference; Deep Blue means "too cold/dark," and Hot Cyan means "too hot/bright."
- **Stereo (Right)**: Displays **Stereo Correlation** from **-1.0 to +1.0**. Use this to ensure mono compatibility and proper width.
- **Loudness (Bottom)**: Tracks **Integrated & Momentary LUFS** (dBFS). Ensure your loudness matches your target genre without clipping.
- **Punch (Left)**: Measures **Dynamic Range (dB)** and **Crest Factor**. This tells you if your mix is too compressed or has enough transient "punch."

### Mix Signature (Session Tracking)
In 2.2.5 Mode, the **Candlestick Meters** track your mix's movement over time.
- **Blue Bars**: Your live mix's current state.
- **Green Targets**: The ideal range based on your selected reference genre.
- **Action**: If your Blue bar is consistently outside the Green target, check the **Fix List** for specific advice.

---

## 3. Working with Aifred (The AI Brain)
Aifred is your "Professional Mentor" integrated directly into the VST.
- **Ask Aifred**: Use the **Chat** tab to ask technical questions like *"Why is my low-end collapsing in mono?"*
- **Context-Aware**: Aifred "sees" your live DSP metrics and provides direct, actionable advice without you having to leave the UI.

---

## 4. Best Practices
1. **Select a Reference**: Always choose a **Reference Genre** (Pop, EDM, etc.) from the Top Bar to ground the AI's feedback.
2. **The "Green" Goal**: Aim to keep the dominant Halo zones and Candlestick meters within the Green target areas.
3. **Session Consistency**: Use the **Last 10 Sessions Candle** to ensure your mixes are consistent across different projects.
