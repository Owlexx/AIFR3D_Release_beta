# AI Router Spec (Canonical)

Version: 1.0
Date: 2026-03-02

## 1) Purpose
Define one provider-agnostic routing contract for AI features across:
- Website
- Android admin app
- Desktop standalone
- VST/Plugin path (via daemon/companion)
- Local daemon/service and edge worker

## 2) Core Types

### Tier
- `FREE`
- `ADVANCED`

### Provider
- `OFFLINE_LLAMA`
- `OPENAI_BYO`

### Message
```json
{ "role": "system|user|assistant", "content": "string" }
```

### Router Response
```json
{
  "text": "string",
  "provider": "OFFLINE_LLAMA|OPENAI_BYO",
  "latency_ms": 0,
  "error": null,
  "meta": {
    "model": "string",
    "tier": "FREE|ADVANCED",
    "last_successful_provider": "OFFLINE_LLAMA|OPENAI_BYO",
    "offline_ready": true,
    "openai_key_present": false,
    "openai_key_valid": false
  }
}
```

## 3) Core Functions

### `chat(messages, options) -> Response`
- Input:
  - `messages`: ordered chat messages
  - `options`:
    - `tier`
    - `preferredProvider` (optional)
    - `offlineBackupMode` (boolean)
    - `sessionId` (optional)
    - `model` (optional)
- Output: `Router Response`

### `analyzeTrack(input, options) -> Response`
- Input:
  - track/path/analysis payload
  - same routing options as `chat`
- Output: `Router Response`
- Note: may be stubbed where unsupported; must still return canonical `provider`, `latency_ms`, and `error` fields.

## 4) Required Settings Keys
Every app/runtime must expose and persist these keys:
- `tier`: `FREE|ADVANCED`
- `preferredProvider`: `OFFLINE_LLAMA|OPENAI_BYO`
- `offlineBackupMode`: boolean
- `llamaEndpoint`: default `http://localhost:11434`
- `llamaModel`: default `llama3`
- `openaiKeyPresent`: boolean
- `openaiKeyValid`: boolean
- `lastSuccessfulProvider`: `OFFLINE_LLAMA|OPENAI_BYO|""`

## 5) Routing Rules (Hard)
1. If `tier=FREE`:
   - MUST route to `OFFLINE_LLAMA` only.
   - MUST NOT attempt `OPENAI_BYO`.

2. If `tier=ADVANCED`:
   - If user BYO OpenAI key is present and valid, route to `OPENAI_BYO`.
   - If key missing/invalid, route to `OFFLINE_LLAMA` and return banner message in `meta`:
     - `"Advanced Chat requires your own OpenAI key. Otherwise you're on Offline Mode (LLaMA)."`

3. If `offlineBackupMode=true`:
   - Force route to `OFFLINE_LLAMA` regardless of tier.

4. No global/shared/project key fallback:
   - No app may ship or silently use owner OpenAI keys.

## 6) Error Handling
Standardized error codes:
- `OFFLINE_UNAVAILABLE` (Ollama not reachable)
- `OPENAI_KEY_MISSING`
- `OPENAI_KEY_INVALID`
- `PROVIDER_TIMEOUT`
- `PROVIDER_HTTP_ERROR`
- `ROUTER_CONFIG_ERROR`

Error response requirements:
- `text` should be human-readable and non-judgmental.
- `provider` should be the attempted provider.
- `latency_ms` always set.
- `error` includes machine code and summary.

## 7) Fallback Behavior
- Primary behavior is rule-based routing above.
- Fallback chain:
  - `FREE`: no provider fallback beyond retrying offline once.
  - `ADVANCED`:
    - Try `OPENAI_BYO` once when key valid.
    - On key/auth failure -> mark key invalid and route to offline.
    - On transport failure -> optional single retry, then route to offline.
- All fallbacks must update `lastSuccessfulProvider` when successful.

## 8) Telemetry / Instrumentation (local)
Each call records:
- provider used
- tier used
- latency_ms
- error code (if any)
- model name
- timestamp

Must not record:
- raw API keys
- plaintext secrets

## 9) UI Copy (required)
Show exactly this user-facing statement in settings/help areas:
- `Advanced Chat requires your own OpenAI key. Otherwise you're on Offline Mode (LLaMA).`

## 10) Security Notes
- Never hardcode secrets in client bundles.
- BYO OpenAI key storage is platform-secure where available.
- VST/plugin path must not embed keys; use daemon/companion or session-only memory.
