# AIFR3D VST | Developer Architecture Spec

This document outlines the technical architecture of the AIFR3D VST system, focusing on the "Clean Wiring" and modularization of the backend and UI.

---

## 1. System Architecture
AIFR3D is a **Dual-Core System** designed for scalability and professional production.

### Dual-Core UI Implementation
The UI is built on a **Modular Translation Layer** in the `Source/UI` and `Source/UI_v25` directories.
- **`PluginEditor`**: The central orchestrator that hosts the UI. It uses a `bool isV25Mode` toggle to dynamically swap between the 2.2.4 and 2.2.5 interface sets.
- **`TopBar`**: Contains the version toggle logic, which triggers the `onVersionToggleChanged` callback in the `PluginEditor`.
- **Dynamic Resizing**: The `resized()` function in `PluginEditor.cpp` handles the layout for both versions, ensuring a seamless experience.

### Isolated DSP Logic
To ensure bit-perfection, the DSP logic for both versions is strictly isolated:
- **`Source/DSP/`**: Canonical 2.2.4 analysis engine (Baseline EBUR128).
- **`Source/DSP_v25/`**: Canonical 2.2.5 analysis engine (Refined EBUR128 + Macro Correlation).

---

## 2. Backend Modularization
The backend has been reorganized into a **Service-Oriented Architecture** for the `AIFR3D_VST_2.2.5` and `AifredVst-2.2.4-beta` repositories.

### Directory Structure
- **`server/src/config/`**: Centralizes all project paths, API tokens, and secrets in `constants.js`.
- **`server/src/services/`**: Contains dedicated logic for `chatService` (AI), `catalogService` (Beats), and `paymentService`.
- **`server/src/routes/`**: Clean API endpoints for the VST and Website to interact with the server.
- **`server/src/middleware/`**: Security gateway for API tokens and Admin authorization.
- **`server/src/utils/`**: Common helpers for safe file operations and system stability.

---

## 3. The "Dawai" Advisory Layer
The `Source/dawai/` directory contains the C++ headers for the **Aifred Brain** integration.
- **`advisory_service.hpp`**: The main bridge for AI feedback.
- **`openai_adviser.hpp`**: The direct link to the LLM (OpenAI API).
- **`memory_store.hpp`**: Stores session history to provide context-aware AI feedback.

---

## 4. Technical Constants
- **API Base**: `/api/v1`
- **Default Port**: `8787`
- **Primary AI Model**: `gpt-5.2`
- **Database**: JSON-based persistent storage in `server/storage/`.
