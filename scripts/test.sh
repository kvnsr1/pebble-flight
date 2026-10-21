#!/bin/sh
set -eu
PROJECT_DIR=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
TOOLS_DIR=${PEBBLE_TOOLS_DIR:-"$PROJECT_DIR/.tools"}
export PATH="$TOOLS_DIR/node/bin:$TOOLS_DIR/bin:$PATH"
cd "$PROJECT_DIR"
node test/flight.test.js
node test/multiflight.test.js
node test/settings-page.test.js
node test/prototype_mode.test.js
TEST_BINARY=$(mktemp "${TMPDIR:-/tmp}/pebble-prototype-test.XXXXXX")
trap 'rm -f "$TEST_BINARY"' EXIT HUP INT TERM
cc -DPEBBLE_MOCK_DATA=1 -std=c99 -Wall -Wextra -Werror test/prototype.test.c \
  src/c/prototype/navigation.c src/c/prototype/mock_flights.c \
  src/c/prototype/booking.c src/c/prototype/flight_model.c -o "$TEST_BINARY"
"$TEST_BINARY"
cc -std=c99 -Wall -Wextra -Werror test/motion.test.c \
  src/c/prototype/motion.c -o "$TEST_BINARY"
"$TEST_BINARY"
node test/native-view.test.js
node test/live-sync.test.js
node test/launcher-glance.test.js
cc -std=c99 -Wall -Wextra -Werror test/glance_plan.test.c \
  src/c/prototype/glance_plan.c -o "$TEST_BINARY"
"$TEST_BINARY"
cc -std=c99 -Wall -Wextra -Werror test/live_store.test.c \
  src/c/prototype/live_store.c -o "$TEST_BINARY"
"$TEST_BINARY"
