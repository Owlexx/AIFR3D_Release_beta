# API Limits Log

Last updated: 2026-03-01

## Global
- Chat provider: Cloudflare Workers AI
- Online mode: required (no offline chat fallback)
- Persistent memory window: last 40 interactions per session
- Primary brain config: `assets/aifr3d_brain/brain_config.json`

## Chat Endpoint Limits
- Endpoint: `POST /api/v1/chat/ask`
- Prompt max length: 4000 characters
- Max tokens per model attempt: 900
- Temperature: 0.25
- Max model retries per request: 6
- Structured response required keys:
  - `summary`
  - `issues`
  - `action_suggestions`
  - `state_update`

## Model Fallback Order (default)
1. `@cf/openai/gpt-oss-120b`
2. `@cf/openai/gpt-oss-20b`
3. `@cf/qwen/qwen2.5-coder-32b-instruct`
4. `@cf/mistralai/mistral-small-3.1-24b-instruct`
5. `@cf/meta/llama-3.3-70b-instruct-fp8-fast`

## Command Endpoint Limits
- Endpoint: `POST /api/v1/command/run`
- Cloud worker behavior: simulated command allowlist only
- Local server behavior: bounded child process execution with timeout
- Timeout target: 15000 ms

## File Upload Limits
- Endpoint: `POST /api/v1/files/upload`
- Local server file size limit: 512 MB per file
- Admin soundpack worker memory list cap: 200 items

## Observability
- Runtime API limit snapshot endpoint: `GET /api/v1/limits`
- Health endpoint includes active model list: `GET /api/v1/health`
