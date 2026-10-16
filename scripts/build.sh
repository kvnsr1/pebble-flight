#!/bin/sh
set -eu

PROJECT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
TOOLS_DIR=${PEBBLE_TOOLS_DIR:-"$PROJECT_DIR/.tools"}
export PATH="$TOOLS_DIR/node/bin:$TOOLS_DIR/bin:$PATH"

cd "$PROJECT_DIR"
exec pebble build

