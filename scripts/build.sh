#!/bin/sh
set -eu

PROJECT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
export PATH="$PROJECT_DIR/.tools/node/bin:$PROJECT_DIR/.tools/bin:$PATH"

cd "$PROJECT_DIR"
exec pebble build

