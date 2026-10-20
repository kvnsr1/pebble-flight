The plane drawn in `src/c/prototype/pebble_plane.c` is a native GPath
translation of **Pebble_25x25_Scheduled_flight.svg**, originally from PebbleOS,
provided by [pebble-dev/iconography](https://github.com/pebble-dev/iconography).
The original SVG and Apache-2.0 license are retained alongside this notice.
Modification: SVG polygon, open polyline and cockpit line translated to Pebble
GPath/drawing primitives; the half-pixel wing coordinate is rounded to 15.
The icon is drawn at its original 25×25 size, without bitmap scaling or rotation.

This GPath remains the neutral logo/empty-state and allocation-failure fallback.
The Overview progress marker now uses the user's supplied 50/80 px SVG derivatives,
converted to PDC and scaled to the existing marker footprint. Sources, conversion
details and native scaling are documented in `resources/progress-plane/README.md`.
