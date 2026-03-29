# AIFR3D Repo Split Blueprint

Date: 2026-03-13

## Goal

Keep the public website live, but move website source, Cloudflare worker code, Wrangler deploy config, and the Android admin app out of the public distribution path.

## Target Repos

### 1. Public Product Repo

Purpose:
- VST source
- Standalone source
- user-facing docs
- release packaging
- install scripts that consume signed release artifacts instead of cloning source

Keep:
- `Source/`
- `modules/`
- `apps/dawai/`
- `assets/`
- user-facing portions of `docs/`
- build scripts required for desktop products

Do not ship:
- `apps/website/`
- `apps/android_admin/`
- `cloudflare/`
- `wrangler.toml`
- admin/deploy credentials or private tooling

### 2. Private Control Plane Repo

Purpose:
- public website source
- Cloudflare worker/admin worker
- Wrangler config and deploy scripts
- Android admin app
- admin chat/tool runner
- private ops docs

Keep:
- `apps/website/`
- `apps/android_admin/`
- `cloudflare/`
- `workers/`
- `wrangler.toml`
- private deploy scripts
- admin/API config

Also copy in:
- `AGENTS.md`
- `docs/dsp_math_pack_v1.md`
- `docs/schema/report.schema.json`
- reference-pool scripts and data contracts
- any analyzer code/docs the Android app needs to stay aligned with canonical DSP behavior

## Canonical DSP / Reference Assets

These stay canonical and must be mirrored into the private control plane repo for the Android analyzer and admin agent:

- `AGENTS.md`
- `docs/dsp_math_pack_v1.md`
- `docs/schema/report.schema.json`
- `scripts/build_licensed_reference_pool.py`
- `scripts/validate_reference_pool_sources.py`
- reference pool source/config files

Rule:
- Desktop and Android analysis must point to the same formulas and reference-pool contracts.
- Do not create a second unofficial DSP math document.

## Public Website Rules

- No admin UI in public HTML.
- No browser-stored admin credentials.
- No browser-stored OpenAI API keys for admin chat.
- No Android admin APK link.
- No public installer that clones the full source repo.

## Private Admin Agent Rules

- Android app talks to private admin APIs only.
- OpenAI tool use runs server-side through the Responses API.
- Cloudflare and Wrangler secrets stay server-side or in a private admin-only control surface.
- Model registry is editable by admin, but public website does not expose it.
- Tool registry must support:
  - file read/write/delete
  - repo search/diff/status
  - controlled shell commands
  - Cloudflare deploy actions
  - website content update actions
  - image generation
  - code scanning
  - audio analysis
  - website fetch/inspection

## Android Terminal Constraint

“Exactly like Termux” requires a real Android-side executor.

Acceptable implementation paths:
- integrate with Termux/Termux:API on-device
- ship a dedicated local command service inside the admin app
- expose a controlled remote executor through the private admin backend

Not acceptable:
- pretending model tool calls alone grant unrestricted phone shell access

## Immediate Next Steps

1. Replace public repo-clone installers with artifact installers.
2. Split public website worker and private admin/control worker.
3. Move admin chat off the website and into the Android app only.
4. Add model registry and command registry APIs in the private control plane.
5. Build the Android app around separate tabs for chat, analyzer, catalog, and commands.
