# DawAI Architecture (Canonical)

Version baseline: 2.2.4 (canonicalized on 2026-03-04)

## 1) Product Surfaces

- Desktop plugin/UI stack (JUCE): `Source/`
- Desktop standalone app: `apps/dawai` + `modules/*`
- Backend API server: `server/src/index.js`
- Website frontend: `apps/website`
- Cloud runtime worker: `cloudflare/website-worker.mjs`
- Android admin app: `apps/android_admin`

## 2) Core Modules

- Metering: `modules/metering`
- Audio engine + transport + plugin host: `modules/audio_engine`
- Reference profiling/delta/genre detection: `modules/reference_engine`
- Feature/scoring/fix writer: `modules/aifr3d_core`
- Advisory + memory: `modules/advisory_layer`
- Standalone UI panels: `modules/ui`

## 3) Chat Architecture (Cloud/OpenAI)

- Primary API route: `POST /api/v1/chat/ask`
- Websocket route: `/ws/chat`
- Active provider contract: OpenAI Responses API (cloud)
- Model list route: `GET /api/v1/models/list`
- Chat behavior requirement:
  - no static canned path as primary response source
  - response must be grounded in current analysis output + session memory context

## 4) DSP and Metering Contract

- Deterministic math source of truth:
  - `docs/dsp_math_pack_v1.md`
  - `docs/schema/report.schema.json`
- Metering metrics include:
  - LUFS (integrated/short-term), true peak, RMS/crest, stereo width/correlation, spectral bands
- Realtime safety policy:
  - audio callback path: no blocking I/O, no file writes, no locks, no heap allocation
  - callback emits tap frames into lock-free queue/ring
  - analysis thread computes metering + scoring and publishes UI snapshot

## 5) Session Analytics + Candlestick Contract

- Candlestick semantic unit: 1 candle = 1 mix session
- Rolling retention: last 10 sessions only
- Session boundaries must be explicit start/end events
- Candle values represent session-over-session deviation from reference/baseline metrics
- Storage policy:
  - bounded persistence only (no unbounded append)
  - schema version required
  - rotate/expire logs by size and/or TTL

## 6) Control Plane

- Website + Android admin share backend contracts for:
  - admin auth/session verify
  - uploads, files/content management, catalog, inquiries, sales, receipts
- Admin surfaces must not require manual absolute path typing for normal operations.

## 7) Canonical Documentation Policy

- Canonical ledger: `AGENTS.md`
- Canonical architecture document: `docs/architecture.md`
- `README.md` is user-facing only.
- Overlapping private logs (`agent.md`, `workers.md`) are deprecated and archived under:
  - `/_archive/noncanonical/2026-03-04/`
