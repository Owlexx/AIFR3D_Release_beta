# AI Tier Plan

Date: 2026-03-02

## Step 0 Discovery Summary

### Detected apps and stacks
- Website client: plain HTML/CSS/JavaScript (`apps/website/index.html`, `apps/website/app.js`), no React/Next build framework.
- Website/backend API: Node.js + Express + `ws` (`server/src/index.js`).
- Website edge runtime: Cloudflare Worker (`cloudflare/website-worker.mjs`).
- Android admin app: Kotlin + Jetpack Compose (`apps/android_admin/app/src/main/java/com/dawai/admin/MainActivity.kt`).
- Desktop standalone + VST: JUCE C++ (`Source/*`, `modules/*`, `apps/dawai/*`, top-level CMake).
- Local daemon/service: JUCE HTTP/WS daemon in `apps/dawai/src/daemon_server.cpp` and Node API daemon in `server/src/index.js`.

### Current AI/chat wiring detected
- Website and Android call `/api/v1/chat/ask` and `/ws/chat`.
- Node backend currently supports OpenAI, Cloudflare, and Ollama logic.
- Cloudflare worker currently supports Cloudflare model routing.
- Desktop advisory path uses `LocalBrainAdviser` and `OllamaAdviser`, with cloud API call path available.

### Implementation points for unified AI router
- Canonical spec docs:
  - `/docs/AI_ROUTER_SPEC.md`
- Node backend router (shared HTTP/WS behavior for website/android/desktop clients):
  - `server/src/index.js`
- Cloudflare worker router parity:
  - `cloudflare/website-worker.mjs`
- Website settings + tier UX + provider status:
  - `apps/website/index.html`
  - `apps/website/app.js`
- Android settings + secure BYO key storage + provider status:
  - `apps/android_admin/app/src/main/java/com/dawai/admin/MainActivity.kt`
- Desktop/standalone/VST router and tier settings:
  - `apps/dawai/src/daemon_server.cpp`
  - `modules/advisory_layer/*`
  - `Source/UI/MainView.*`
  - `modules/ui/src/main_view.cpp`

### Prerequisites / gaps
- Ollama runtime:
  - `ollama` binary is present on this machine.
  - `llama.cpp` runtime binaries are not present (`llama-server` missing).
- Secure key storage libraries:
  - Android encrypted storage dependency not yet wired.
  - Desktop keychain/keytar support not currently present in JUCE C++ path; companion daemon approach is needed.
- Existing routes and admin endpoints are active and must stay behavior-compatible.

### Sequential implementation order (locked)
1. Step 1: write canonical router spec.
2. Step 2: implement offline LLAMA provider baseline + health checks.
3. Step 3: implement BYO OpenAI key storage/validation per platform.
4. Step 4: wire offline-first AI router and fallback rules.
5. Step 5: unify settings UX copy and tier gating.
6. Step 6: website preview player + diagnostics visual cleanup + secure promo/admin endpoints.
7. Step 7: repo-wide secret hygiene + remediation doc.
8. Step 8: smoke checks + final commits.
