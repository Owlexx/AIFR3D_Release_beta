# AIFR3D 2.2.4 Beta Session Change Log

Date: 2026-03-09

## Audit
- Analysis metrics are calculated in `Source/DSP/AnalysisEngine.cpp`.
- Scoring is calculated in `Source/DSP/ScoringEngine.cpp`.
- UI reads interpreted results in `Source/UI/UiModel.cpp`, `Source/UI/MainView.cpp`, `Source/UI/ApprovalHalo.cpp`, and `Source/UI/MixSignatureMeter.cpp`.
- Chat/provider/model wiring is configured in `server/src/index.js`, `cloudflare/website-worker.mjs`, `apps/website/app.js`, and `apps/android_admin/app/src/main/java/com/dawai/admin/MainActivity.kt`.

## Core Product Changes
- Locked AIFR3D advisory/chat flows to OpenAI-only paths with GPT-5.2 primary support and GPT-5-nano allowlist where requested.
- Added canonical `MixMetrics` wiring and `AifredReport` interpretation flow for UI consumption.
- Expanded analysis metric coverage with frequency-balance, integrated-loudness, and dynamic-range metadata/config integration.
- Tightened diagnostic issue generation so mud, harshness, bass-thin, hollow, loudness, and compression states map directly into the fix list.

## DSP + UI Stability
- Changed plugin analysis from per-block processing to short burst windows throughout playback in `Source/Plugin/PluginProcessor.cpp`.
- Re-enabled short UI smoothing ramps in `Source/UI/UiModel.cpp` so meters stay smooth between burst updates.
- Kept persistent mix-history memory limited to the last 10 sessions through the candlestick session stores.
- Kept fix-list history capped and session-scoped.
- Changed chat wording to session-only history in `Source/UI/MainView.cpp`.

## Plugin / Standalone GUI
- Updated the halo to the four-zone diagnostic layout with tone, stereo, loudness, and dynamics labeling in `Source/UI/ApprovalHalo.cpp` and `Source/UI/ApprovalHalo.h`.
- Added pulse-mode-aware outer ring behavior and soft background analyzer rendering.
- Added candlestick metric meters in `Source/UI/MixSignatureMeter.cpp`.
- Added `Mix Tips` tabs and panel in `Source/UI/MixTipsPanel.h`, `Source/UI/MixTipsPanel.cpp`, `Source/UI/MainView.h`, and `Source/UI/MainView.cpp`.
- Added startup splash support and tutorial updates in `Source/Plugin/PluginEditor.cpp`, `Source/Plugin/PluginEditor.h`, `apps/dawai/src/main.cpp`, and `apps/dawai_standalone/main.cpp`.
- Added splash asset `assets/branding/mascot_splash_2_2_4_beta.png`.

## Website
- Reworked the website GUI and branding surfaces in `apps/website/index.html`, `apps/website/styles.css`, and `apps/website/app.js`.
- Added educational metric content page and tabs using `apps/website/assets/data/analysis_metric_catalog.json`.
- Fixed beat-catalog playback routing to site-hosted audio assets and synced catalog audio via `scripts/sync_website_catalog_audio.sh`.
- Limited website chat history to the current session and capped DOM history growth in `apps/website/app.js`.
- Removed obsolete websocket config from `apps/website/config.js`.
- Updated worker asset/chat behavior in `cloudflare/website-worker.mjs`.

## Android Admin App
- Kept the app at 3 tabs: Chat, Upload, Command.
- Updated title/version labeling to `AIFR3D Admin 2.2.4 Beta`.
- Added audio permission and player/analysis wiring updates.
- Changed chat memory to one launch-scoped session ID only.
- Added chat cleanup on app close and server memory clear calls in `apps/android_admin/app/src/main/java/com/dawai/admin/MainActivity.kt`.
- Capped in-app chat history to a bounded session list.

## Server / Backend
- Added session-scoped memory clearing in `server/src/index.js`.
- Added `POST /api/v1/memory/clear`.
- Added websocket `chat.clear` handling.
- Trimmed stored admin sessions before appending new ones.
- Preserved catalog upload target at `server/data/North3rnlighter beats`.
- Preserved inquiry target email routing to `north3rnlight3rofficial@outlook.com`.

## Reference Pool / Metric Catalog
- Added `assets/reference_intake/analysis_metric_catalog.json`.
- Added metric-catalog embedding to reference-pool builders in `scripts/build_licensed_reference_pool.py` and `scripts/build_desktop_reference_pool.py`.
- Extended reference profile serialization in:
  - `modules/reference_engine/include/dawai/reference_engine/reference_profile.hpp`
  - `modules/reference_engine/src/profile_cache.cpp`
  - `modules/reference_engine/src/reference_library.cpp`
  - `modules/reference_engine/src/reference_profiler.cpp`
- Updated `assets/reference_intake/licensed_pool_config.template.json`.
- Added validation coverage in `tests/profile_cache_test.cpp`.

## Packaging / Install
- Updated Linux install/package scripts:
  - `scripts/install_local_linux.sh`
  - `scripts/install_arch.sh`
  - `scripts/create_linux_packages.sh`
- Included runtime splash asset installation in Linux installs/packages.
- Added/updated Android admin guide and command list docs:
  - `docs/android-admin-app-guide.md`
  - `docs/android-admin-command-list.txt`

## New Files Added
- `Source/Common/MixMetrics.h`
- `Source/Interpretation/AifredReport.h`
- `Source/Interpretation/DiagnosticEngine.h`
- `Source/Interpretation/DiagnosticEngine.cpp`
- `Source/UI/MixTipsPanel.h`
- `Source/UI/MixTipsPanel.cpp`
- `apps/website/assets/brand/hero_mascot.png`
- `apps/website/assets/data/analysis_metric_catalog.json`
- `assets/branding/mascot_splash_2_2_4_beta.png`
- `assets/reference_intake/analysis_metric_catalog.json`
- `docs/android-admin-app-guide.md`
- `docs/android-admin-command-list.txt`
- `scripts/sync_website_catalog_audio.sh`
- `scripts/validate_reference_pool_sources.py`

## Validation Completed
- `cmake --build build --target aifred_vst3_A --parallel`
- `cmake --build build --target dawai_tests --parallel`
- `ctest --test-dir build --output-on-failure -R 'aifred_spectral_pipeline_smoke|aifr3d_fix_list_reactivity_smoke'`
- `./build/tests/dawai_tests '[reference]'`
- `node --check apps/website/app.js`
- `node --check cloudflare/website-worker.mjs`
- `node --check server/src/index.js`
- `env GRADLE_USER_HOME=/tmp/gradle-home ./gradlew :app:assembleDebug`

## Release Notes
- Chat history is now session-only.
- Persistent product memory is limited to mix-history candles over the last 10 sessions.
- DSP metering now runs in short overlapping bursts during playback to reduce spikes while keeping meter accuracy responsive.
- Website catalog audio sync now writes `asset_file_name`, `public_url`, `full_song_url`, and `stream_url` against deployable local asset paths, with oversized sources transcoded to `.preview.mp3`.
- Cloudflare worker seed loading now preserves catalog asset URLs instead of rewriting them back to raw media URLs.
- Linux VST3 and standalone variants were rebuilt and reinstalled locally from the current repo state.
- Android admin debug APK was rebuilt successfully; device install and document copy are blocked until `adb` authorization is accepted on the phone.
