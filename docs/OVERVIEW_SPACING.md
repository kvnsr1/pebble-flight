# Overview circle spacing — 0.7.2

Validated 19 October 2026. The Overview progress circle grows from **98 px** to
**116 px**, with bounds `(40, 24, 116, 116)` on the 200×228 Emery display. Its
8 px stroke and existing airplane are unchanged. Flight number, percentage,
status and airport/time labels retain their existing positions and fonts.
Navigation, data integration and the other detail pages are unchanged.

The first larger trial crowded the header as the marker traveled around the
arc. The final size was checked through the whole path, including the reported
`WN1145` / 0% case and the opposite endpoint at landing.

- Production build compiled with Pebble Tool 5.0.40 / SDK 4.33.1, live data on.
- All seven existing automated test suites passed.
- Captured **every integer progress value from 0–100%**, plus boarding, delayed,
  canceled, unknown and in-flight/unknown-progress states in the actual Emery
  emulator. Header and flight-number glyphs stay intact at all 101 positions.
- Compared 17 matching Overview frames with 0.7.1: existing content outside
  the progress graphic and status-label positions are unchanged.
- All Flights, Departure, Arrival, Booking and restored list match the previous
  build pixel for pixel. Native physical-button events opened/reversed pages
  and returned to the selected list row during capture.
- Test flight/time values were synthetic AppMessage inputs. No live provider
  request, physical-watch installation or authenticated CloudPebble run occurred.
  No appstore release was published.

[Before/after](overview-spacing/before-after.png) ·
[Progress samples](overview-spacing/progress-preview.png) ·
[Comparison results](overview-spacing/results.json)

Pull `main`, confirm version **0.7.2**, rebuild and install through CloudPebble's
**Build & Run → Phone** to check the change on the watch.
