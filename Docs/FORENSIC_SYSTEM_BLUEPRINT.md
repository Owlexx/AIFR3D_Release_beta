# AudioSuite System - Forensic System Blueprint & Function Log
**Version**: 2.5.0-beta (Arch/Garuda Optimized)
**Build Status**: Finalized, Cleaned, and Renamed

## **1. Core Philosophy: The Translation Layer**
The AudioSuite system is designed as a professional "Translation Layer" between audio physics and human perception. It consists of four integrated components:
1.  **AudioSuite VST**: A high-precision mixing and analysis plugin for Linux and Windows.
2.  **AudioSuite Standalone**: A specialized AI-driven vocal recording suite.
3.  **AudioSuite Admin App**: A mobile terminal and management tool for Android.
4.  **AudioSuite Backend**: A modular, secure, and real-time Node.js/WebSocket API.

---

## **2. File System & Component Map**
All components have been renamed to use non-Aifred/DawAI identifiers to ensure a clean and unambiguous build environment.

### **A. AudioSuite VST & Standalone (`Source/`)**
| File | Class | Function | Plain English Summary |
| :--- | :--- | :--- | :--- |
| `Plugin/PluginProcessor.cpp` | `CoreSynthAudioProcessor` | `processBlock()` | The "Engine": Handles the actual audio math and analysis in real-time. |
| `Plugin/PluginEditor.cpp` | `CoreSynthAudioProcessorEditor` | `changeListenerCallback()` | The "Coordinator": Receives DSP updates and sends them to the UI and WebSocket. |
| `UI/TopBar.cpp` | `TopBar` | `resized()` | The "Navigation": Manages the version toggle and theme settings button. |
| `UI_v25/ApprovalHalo.cpp` | `ApprovalHalo` | `paint()` | The "Visualizer": Renders the Tone (Hz), Stereo, and Loudness (dBFS) scales. |
| `UI_v25/MixSignatureMeter.cpp` | `MixSignatureMeter` | `update()` | The "Tracker": Visualizes the "Candlestick" session history for dynamic range. |

### **B. AudioSuite Standalone Vocal (`apps/standalone_vocal_recorder/`)**
| File | Class | Function | Plain English Summary |
| :--- | :--- | :--- | :--- |
| `StandaloneVocalRecorder.cpp` | `StandaloneVocalRecorder` | `getNextAudioBlock()` | The "Recorder": Manages live vocal input and recording to disk. |
| `AIVocalProcessor.cpp` | `AIVocalProcessor` | `processVocal()` | The "AI Brain": Applies context-aware auto-tune and FX to the vocal signal. |

### **C. AudioSuite Backend (`server/src/`)**
| File | Module | Function | Plain English Summary |
| :--- | :--- | :--- | :--- |
| `main.js` | `MAIN_SERVER` | `wss.on('connection')` | The "Hub": Handles live WebSocket connections and real-time DSP data. |
| `services/chatService.js` | `ChatService` | `processPrompt()` | The "Interpreter": Translates DSP metrics into AI-driven mixing advice. |
| `config/constants.js` | `CONFIG` | `STORAGE_DIR` | The "Map": Centralizes all project paths and secure API tokens. |

---

## **3. Critical "Clean Wiring" Log**
- **WebSocket Gateway**: Established a real-time link (`/ws/vst`) for streaming live DSP metrics from the VST to the Aifred Brain.
- **Backend Security**: Moved all API tokens to the backend; implemented rate limiting and forensic logging in `chatService.js`.
- **Dual-Mode Logic**: Integrated a version toggle that switches between the 2.2.4 (Discovery) and 2.2.5 (Production) interfaces without breaking DSP logic.
- **File System Renaming**: Performed a forensic rename of all directories and files to eliminate naming conflicts and ensure a professional build.

---

## **4. Repeatability & Consistency**
To ensure consistency across builds, the following rules must be followed:
1.  **DSP Bit-Perfection**: Never modify the analysis engine math unless explicitly documented in a version update.
2.  **Modular Backend**: Always use the `/api/v1` structure for new routes; never hardcode tokens on the frontend.
3.  **Plain English Documentation**: Every new function must include a summary translating the code logic into human perception.

**This blueprint serves as the canonical record of the AudioSuite system as of Mar 29, 2026.**
