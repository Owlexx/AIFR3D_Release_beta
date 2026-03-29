# AIFR3D VST 2.3 & 2.2.5 - Verbose Plugin Map

This document serves as the canonical reference for the AIFR3D VST system architecture, meter interpretation, and operational workflow.

## 1. System Overview
AIFR3D is a **Translation Layer** between raw audio physics and human musical perception. It operates in two distinct modes, switchable via the **V2.2.5 Mode** toggle in the Top Bar.

### 2.2.4 Mode (Discovery Architecture)
*   **Focus**: Initial VST3 implementation, early research into spectral balance.
*   **DSP**: Baseline EBUR128 loudness and FFT-based spectral analysis.
*   **GUI**: Focused on the diagnostic "Halo" as the primary read.

### 2.2.5 Mode (System Architecture)
*   **Focus**: Production-ready precision and the "Dawai" Advisory Layer.
*   **DSP**: Refined EBUR128 math, improved transient detection, and macro-pattern correlation.
*   **GUI**: Enhanced visualization with "Candlestick" session tracking and "Mix Tips" tabs.

---

## 2. Meter Interpretation Guide

### The Diagnostic Halo (The Macro Read)
The Halo provides a 360-degree fast-diagnostic of your mix.
*   **Top (Tone)**: 20Hz - 20kHz Spectral Balance.
    *   *Read*: Compares your live curve to the reference target. 
    *   *Scale*: Logarithmic Hz scale.
*   **Right (Stereo)**: Stereo Correlation & Width.
    *   *Read*: Measures the "motion" and width of the stereo field.
    *   *Scale*: -1.0 to +1.0 (Correlation).
*   **Bottom (Loudness)**: Integrated & Momentary LUFS.
    *   *Read*: Overall perceived volume and true peak safety.
    *   *Scale*: dBFS / LUFS.
*   **Left (Dynamics)**: Crest Factor & Transient Density.
    *   *Read*: Measures the "punch" vs "compression" of the signal.
    *   *Scale*: Dynamic Range (dB).

### Mix Signature (Candlestick Meters)
Located in the 2.2.5 interface, these track your mix's movement over the current session.
*   **Blue Bars**: Your live mix's current state.
*   **Green Targets**: The ideal range based on your selected reference genre.
*   **Interpretation**: If the Blue bar is outside the Green target, the "Fix List" will trigger a specific action suggestion.

---

## 3. Operational Workflow

### Step 1: Initialize
1.  Open the VST on your Master Bus.
2.  Select your **Reference Genre** (Pop, EDM, Hip-Hop, etc.) from the Top Bar.
3.  Choose your **UI Mode** (2.2.4 for legacy view, 2.2.5 for modern tracking).

### Step 2: Analyze
*   Play your loudest section.
*   Watch the **Halo** for immediate alerts (Deep Blue = Too Low/Cold, Hot Cyan = Too High/Hot, Green = Balanced).
*   Review the **Fix List** for dominant issues (e.g., "Mud in the Low-Mids" or "Stereo Width Collapse").

### Step 3: Consult (Aifred Brain)
*   Click the **Chat** tab to ask Aifred specific questions about the diagnostic data.
*   Aifred has a "Professional Mentor" personality and will provide direct, technical advice based on your live metrics.

---

## 4. Backend Wiring & Admin
The VST, Website, and Admin App are all wired to a unified **Modular Backend**:
*   **API Base**: `/api/v1`
*   **Authentication**: Secure API Tokens and Admin session cookies.
*   **Data**: All session data, catalog items, and promo codes are stored in the `server/storage/` directory as JSON "mini-databases."

---

## 5. Developer Notes (Filling in the Code)
*   **DSP Wiring**: Ensure `Source/DSP/` (2.2.4) and `Source/DSP_v25/` (2.2.5) are correctly linked in your CMake build.
*   **GUI Wiring**: The `PluginEditor` dynamically swaps `MainView` and `MainView_v25` based on the `isV25Mode` boolean.
*   **API Key**: You must provide your OpenAI API Key in the VST settings or the `openai_config.json` file on the server to enable Aifred Chat.
