# Security Threat Model

Date: 2026-03-02

## Assets to Protect
- Paid downloads (VST/standalone/packages/beats).
- Admin operations (content edits, file access, catalog mutation, promo controls).
- Promo validation and redemption logic.
- Payment workflows and webhook integrity.
- Customer inquiry data and sales records.
- Session tokens and any entitlement/license state.

## Likely Attacks
- Direct-link scraping of downloadable assets.
- Link sharing/replay of download URLs.
- Promo brute force and automation abuse.
- Admin credential stuffing and brute force.
- PayPal webhook spoofing / replay.
- API flooding and endpoint abuse.
- XSS/CSRF/session misuse.
- Path traversal/injection attempts in file APIs.
- Supply-chain risk through vulnerable dependencies.

## Trust Boundaries
- Public browser/client boundary.
- API boundary (Node/Worker).
- Cloudflare edge + WAF + Turnstile boundary.
- Storage boundary (R2/KV/local files).
- Payment provider boundary (PayPal webhooks).

## Security Posture Target
- Hard to abuse.
- Low maintenance overhead.
- Minimal user friction.
- No privileged secrets in client code.
- Short-lived, signed, entitlement-based download access.
