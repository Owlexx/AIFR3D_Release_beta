#!/usr/bin/env bash
set -euo pipefail

export CORESYNTH_INSTALL_TARGET=vst

BRANCH="${CORESYNTH_BRANCH:-AudioSynth-Vst-Beta-2.2.4}"
REPO_URL="${CORESYNTH_REPO_URL:-https://github.com/kaeganscott26/AudioSynthVst-2.2.4-beta.git}"
TARGET_DIR="${1:-$HOME/CoreSynth_Ecosystem}"

log() {
  printf '[plugin-linux-installer] %s\n' "$1"
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

log "Running plugin installer"
bash "$TARGET_DIR/scripts/install_local_linux.sh"
