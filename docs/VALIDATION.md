# Validation — 16 October 2026

## Environment and build

- Native C / PebbleKit JS project, SDK metadata `3`, only `emery`.
- Installed Pebble Tool **5.0.40**, active SDK **4.33.1**, Apple Silicon macOS.
- Default mock-mode `pebble build`: **passed**. Final RAM footprint **7,742
  bytes / 128 KB**, resources **4,342 bytes / 256 KB**, available heap **123,330
  bytes**. Output: `build/pebble-flight.pbw`.
- Preserved live fallback (`PEBBLE_UI_PROTOTYPE=0`): **compiled and installed**
  successfully. Its original setup screen displayed the phone-service message
  “Open phone settings to add flights and an AeroAPI key”, confirming native
  mode handshake and real companion startup. No key or API lookup was used.
- Restored `PEBBLE_UI_PROTOTYPE=1`, rebuilt and reinstalled. This is the delivered
  source and PBW. Final source compiles without C compiler warnings. The SDK's
  linker emits its existing ELF LOAD-segment RWX warning in both modes.

## Automated checks

`sh scripts/test.sh` passed all four suites:

1. Existing flight transformation/delay/refresh rules (`flight.test.js`).
2. Existing multi-flight settings, booking and seat behavior
   (`multiflight.test.js`, now exercising the moved `live.js` service).
3. Phone service isolation: no production require/storage/network before live
   handshake; duplicate replies do not start it twice (`prototype_mode.test.js`).
4. Native C fixture coverage, progress semantics, signed early/late examples,
   overnight local dates, field presence, list/detail limits, reverse navigation,
   note behavior, selection/scroll restoration and empty toggling
   (`prototype.test.c`, host compiler with `-Wall -Wextra -Werror`).

`git diff --check` passed.

## Actual native emulator review

Ran the compiled app in the **local SDK Emery QEMU emulator**, at **200×228**.
The review script sends physical-button press/release events through
`pebble emu-button` and captures the native framebuffer through
`pebble screenshot`. These images are not browser approximations.

- Captured and visually inspected **all six scenarios on Overview, Departure,
  Arrival**, plus their selected list states, the empty list and the data note.
- **39 framebuffer comparisons passed**: list start/end limits, detail start/end
  limits for every scenario, reverse page order, Back preserving selection and
  scroll pixels for every row, note open/close, Back directly from a note,
  detail long Select, empty-state safe Up/Down/Select, and empty/filled restoration.
- Scrolled through all six flights with Up/Down and opened each with Select.
- Inspected long airport names, missing gate/baggage, explicit schedule versus
  estimate/actual labels, early/late text, next-day dates and endpoint timezones.
- Confirmed canceled has X / NO JOURNEY and no plane/progress value, upcoming
  and boarding stay at 0%, in-flight shows 62%, and landed shows 100%.
- A visual pass found the 100% label too close to its arc. Reduced that label's
  font, rebuilt, and captured/inspected the landed Overview again. The navigation
  code was unchanged by this final layout adjustment.
- No text clipping or unreadable overlap remained in these fixture screenshots.
  Readability was evaluated on emulator images, not physical hardware.

See [contact sheet](screenshots/contact-sheet.png), individual PNGs in
`screenshots/`, and [machine-readable comparison results](screenshots/navigation-results.json).
Reproduce with `python3 scripts/review-prototype.py` (Pillow and Pebble CLI
required; built mock PBW required). The script leaves the filled list restored.

## Remaining CloudPebble verification

CloudPebble's authenticated import/build/emulator was **not exercised in this
session**. The source uses the existing standard native project layout and build
configuration; a local build does not prove CloudPebble import behavior.

1. Import repository `kvnsr1/pebble-flight`, branch
   `codex/native-flight-ui-prototype`; verify Pebble C SDK / SDK 3 / emery.
2. Confirm `prototype_config.h` has `PEBBLE_UI_PROTOTYPE 1`, compile, choose
   Emulator / Pebble Time 2 (emery), run Flight Preview.
3. Scroll through every flight and inspect all three pages. Up must stop on
   Overview, Down on Arrival, and Back must restore both selected row and scroll.
4. On a detail page, Select opens/closes the data note; Back from the note must
   return directly to All Flights. Hold Select on All Flights to check the empty
   state and hold again to restore the same position.
5. Use CloudPebble's Screenshot control to check the same labels and edge cases
   at its actual rendering scale. Optionally repeat in both browser and cloud
   emulator settings. No credentials are needed.

No live FlightAware network lookup, physical-watch test, appstore release,
GitHub Pages deployment, or default-branch merge was performed. The old live UI
remains available behind the switch; connecting the new list/detail UI to live
production data is intentionally deferred until design approval.
