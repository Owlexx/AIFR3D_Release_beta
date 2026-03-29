# AIFR3D VST | Routing & Build Checklist

This document provides a precise checklist for building and deploying the AIFR3D VST system.

---

## 1. Backend Routing Checklist
Ensure all backend components are correctly wired and accessible.

- [ ] **Modular API**: Verify that the backend is running and listening on the correct port (default: `8787`).
- [ ] **API Token**: Ensure the `DAWAI_API_TOKEN` is set in the environment variables or the `constants.js` file.
- [ ] **Storage Folders**: Confirm that the `server/storage/` directory exists and has write permissions.
- [ ] **Database Files**: Verify that `catalog.json`, `content.json`, and `sessions.json` are correctly initialized in the storage folder.
- [ ] **Static Assets**: Check that the `/assets` route correctly serves images and media from the project root.

---

## 2. VST Build Checklist
Follow these steps to build the AIFR3D VST with the dual-mode GUI.

- [ ] **CMake Configuration**: Ensure that both `Source/DSP/` and `Source/DSP_v25/` are correctly included in the `CMakeLists.txt` file.
- [ ] **JUCE Framework**: Confirm that the JUCE 8.0.6 modules are correctly linked and accessible.
- [ ] **Version Toggle**: Verify that the `TopBar` version toggle correctly switches between `MainView` and `MainView_v25`.
- [ ] **DSP Parity**: Test both modes to ensure the DSP logic for each version is functioning as intended.
- [ ] **Window Resizing**: Confirm that the plugin window correctly resizes when switching between the 2.2.4 and 2.2.5 interfaces.

---

## 3. Website & Admin Wiring Checklist
Ensure the frontend components are correctly connected to the unified backend.

- [ ] **Website Config**: Check that `apps/website/config.js` is pointing to the correct `apiBase` (`/api/v1`) and has the correct `apiToken`.
- [ ] **Admin App**: Verify that the Admin App is correctly wired to the `/api/v1/admin` endpoints.
- [ ] **Checkout Logic**: Test the PayPal and Promo Code integration to ensure payments are correctly recorded in the backend.
- [ ] **Aifred Chat**: Confirm that the chat interface is correctly sending and receiving messages from the `/api/v1/chat/ask` endpoint.

---

## 4. Final Deployment
- [ ] **Commit & Push**: Ensure all changes are committed and pushed to the `AIFR3D_VST_2.2.5` repository.
- [ ] **Production Run**: Start the backend server in production mode using the `run_backend_production.sh` script.
- [ ] **VST Distribution**: Package the built VST3 plugin for distribution to users.
