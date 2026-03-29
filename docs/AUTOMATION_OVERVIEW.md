# Automation Overview

Date: 2026-03-02

## Stack Discovery
- Website frontend: static HTML/CSS/JS (`apps/website`).
- Backend API: Node.js + Express + WebSocket (`server/src/index.js`).
- Cloudflare edge runtime: existing Worker (`cloudflare/website-worker.mjs`) and root `wrangler.toml`.
- Android app: Kotlin + Jetpack Compose (`apps/android_admin`).
- Desktop/VST: C++/JUCE (`Source`, `modules`, `apps/dawai`).

## What This Security/Automation Pass Adds
1. Cloudflare worker scaffold under `workers/security-gateway/`.
2. GitHub Action to deploy Worker using GitHub Secrets.
3. GitHub Action security gate:
   - secret scanning
   - dependency audit
   - repo hygiene checks
4. Secret scanning script and baseline security docs.
5. Repo cleanliness updates (`.editorconfig`, `.gitattributes`, `.gitignore` hardening).
6. Duplicate diagnostics asset cleanup (single canonical image).
7. Manual docs for secure deployment and repo privacy process.

## Secret Handling Policy
- No secrets committed in git.
- Cloudflare credentials only via GitHub Secrets and Worker secrets.
- Frontend never embeds privileged credentials.
