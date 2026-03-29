#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

PAYPAL_ID_FILE="${PAYPAL_ID_FILE:-$HOME/Documents/API_KEY_INDEX/PayPalID.txt}"
PAYPAL_SECRET_FILE="${PAYPAL_SECRET_FILE:-$HOME/Documents/API_KEY_INDEX/SecretPal.txt}"
OPENAI_KEY_FILE="${OPENAI_KEY_FILE:-$HOME/Documents/API_KEY_INDEX/OpenAI API Key.txt}"

extract_client_id() {
  local file="$1"
  if [[ ! -f "$file" ]]; then
    return 1
  fi
  grep -E '^[A-Za-z0-9_-]{40,}$' "$file" | head -n1 | tr -d '\r\n'
}

extract_paypal_secret() {
  local file="$1"
  if [[ ! -f "$file" ]]; then
    return 1
  fi
  local raw
  raw="$(sed -n '1p' "$file" | tr -d '\r\n')"
  raw="$(echo "$raw" | sed -E 's/^[[:space:]]*secret[[:space:]]+//I' | sed -E 's/^id//')"
  echo "$raw"
}

extract_openai_key() {
  local file="$1"
  if [[ ! -f "$file" ]]; then
    return 1
  fi
  grep -oE 'sk-[A-Za-z0-9_-]{20,}' "$file" | head -n1 | tr -d '\r\n'
}

PAYPAL_CLIENT_ID="${PAYPAL_CLIENT_ID:-$(extract_client_id "$PAYPAL_ID_FILE" || true)}"
PAYPAL_CLIENT_SECRET="${PAYPAL_CLIENT_SECRET:-$(extract_paypal_secret "$PAYPAL_SECRET_FILE" || true)}"
OPENAI_API_KEY="${OPENAI_API_KEY:-$(extract_openai_key "$OPENAI_KEY_FILE" || true)}"

if [[ -z "$OPENAI_API_KEY" ]]; then
  echo "Missing OpenAI API key. Put a valid sk- key in '$OPENAI_KEY_FILE' or set OPENAI_API_KEY."
  exit 1
fi

if [[ -z "$PAYPAL_CLIENT_ID" || -z "$PAYPAL_CLIENT_SECRET" ]]; then
  echo "Missing PayPal credentials. Check '$PAYPAL_ID_FILE' and '$PAYPAL_SECRET_FILE'."
  exit 1
fi

export OPENAI_API_KEY
export PAYPAL_CLIENT_ID
export PAYPAL_CLIENT_SECRET
export PAYPAL_ENV="${PAYPAL_ENV:-live}"
export PAYPAL_BUSINESS_EMAIL="${PAYPAL_BUSINESS_EMAIL:-north3rnlight3rofficial@outlook.com}"
export PORT="${PORT:-8787}"
export DAWAI_DEFAULT_API_URL="${DAWAI_DEFAULT_API_URL:-https://north3rnlight3r.com}"

echo "Starting backend on :$PORT (PAYPAL_ENV=$PAYPAL_ENV, API URL=$DAWAI_DEFAULT_API_URL)"
exec node "$ROOT_DIR/server/src/index.js"
