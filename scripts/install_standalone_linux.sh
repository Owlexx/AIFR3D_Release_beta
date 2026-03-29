#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DAWAI_INSTALL_TARGET=standalone "$ROOT_DIR/scripts/install_local_linux.sh"
