#!/usr/bin/env bash
set -euo pipefail

BRANCH="${DAWAI_BRANCH:-canonical-224-audit}"
DEFAULT_REPO_URL="https://github.com/kaeganscott26/DawAI_Ecosystem.git"
REPO_URL="${DAWAI_REPO_URL:-$DEFAULT_REPO_URL}"
TARGET_DIR="${1:-$HOME/DawAI_Ecosystem}"
INSTALL_TARGET="${DAWAI_INSTALL_TARGET:-all}"

log() {
  printf '[bootstrap-linux] %s\n' "$1"
}

if ! command -v git >/dev/null 2>&1; then
  echo 'git is required' >&2
  exit 1
fi

if [[ -d "$TARGET_DIR/.git" ]]; then
  log "Updating existing clone in $TARGET_DIR"
  git -C "$TARGET_DIR" fetch origin "$BRANCH"
  git -C "$TARGET_DIR" checkout "$BRANCH"
  git -C "$TARGET_DIR" pull --ff-only origin "$BRANCH"
else
  log "Cloning $REPO_URL#$BRANCH into $TARGET_DIR"
  git clone --branch "$BRANCH" --single-branch "$REPO_URL" "$TARGET_DIR"
fi

log "Running installer (target=$INSTALL_TARGET)"
DAWAI_INSTALL_TARGET="$INSTALL_TARGET" bash "$TARGET_DIR/scripts/install_local_linux.sh"
log 'Complete.'
