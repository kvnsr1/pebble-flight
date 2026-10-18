# Booking icon update — 17 October 2026

Only the boarding-pass and seat-position image regions on detail page four
changed. The labels, text positions, field values, seat inference/override rules,
navigation and other screens are unchanged. This uses the existing native C SDK
project, Emery platform, production UUID and live-data default.

All six user-supplied SVGs (two icons, each in 25/50/80 px variants) are retained
unaltered beside their converted PDCs in
[`resources/booking-icons`](../resources/booking-icons/README.md). Conversion used
the requested HBehrens `pdc_tool` v0.3.4. SVG and resulting PDC rendered identically
for each of the six inputs. The app bundles the 25 px boarding-pass and 80 px
seat/window images as raw resources; CloudPebble does not need the converter.

## Validation

- All six existing automated suites passed (`sh scripts/test.sh`).
- Compiled both mock and normal live builds using Pebble Tool 5.0.40 / SDK 4.33.1.
  No C compiler warnings; the SDK's existing ELF LOAD-segment RWX linker warning
  remains. Live RAM footprint: 35,679 bytes, heap: 95,393 bytes; resources: 5,360
  bytes. The two bundled PDCs add 1,018 resource bytes.
- Used clean copies of tracked source for builds; pre-existing untracked duplicate
  files in the local checkout were not included or modified.
- Captured the baseline (`805f848`) and updated mock app in the actual local
  Emery QEMU emulator at 200×228. All 31 non-Booking captures match pixel for
  pixel: six flights' lists, overviews, departures, arrivals and notes, plus empty.
- All six Booking captures match outside the two image regions. This verifies
  every label, booking code, seat value, position label, footer and page indicator
  is unchanged. Long booking codes and unknown/unavailable fields remain readable.
- Checked 18 seat-cushion samples across six scenarios: only the correct
  window/middle/aisle seat is accented; unknown and unavailable select none.
  Visually inspected the check mark and icon fit in all six captures.
- 24 native framebuffer comparisons passed: last-page Down boundary, Select
  note open/close, Up to Arrival and Back restoring list selection/scroll, for
  every mock scenario.

Evidence: [six Booking screenshots](booking-icons/booking-scenarios.png) and
[comparison results](booking-icons/results.json). Individual PNGs are native
screenshots; the contact sheet only arranges them for viewing.

No live provider lookup, real credentials, physical-watch installation,
authenticated CloudPebble test or appstore publication was performed for this
asset update. Pull `main` in CloudPebble and build/run for Emery; the checked-in
configuration continues to use live data.
