#!/bin/sh
set -eu
PROJECT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
TOOLS_DIR=${PEBBLE_TOOLS_DIR:-"$PROJECT_DIR/.tools"}
export PATH="$TOOLS_DIR/node/bin:$TOOLS_DIR/bin:$PATH"
cd "$PROJECT_DIR"
node test/flight.test.js
node test/multiflight.test.js
node test/prototype_mode.test.js
TEST_BINARY=$(mktemp "${TMPDIR:-/tmp}/pebble-prototype-test.XXXXXX")
trap 'rm -f "$TEST_BINARY"' EXIT HUP INT TERM
cc -std=c99 -Wall -Wextra -Werror test/prototype.test.c \
  src/c/prototype/navigation.c src/c/prototype/mock_flights.c -o "$TEST_BINARY"
"$TEST_BINARY"
