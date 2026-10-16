# Validation — Pebble Flight 0.7.0

Reviewed 16 October 2026. The approved interface is now the default live app;
[the prototype-only validation history](PROTOTYPE_VALIDATION.md) is retained.

## Build and identity

- Pebble Tool **5.0.40**, SDK **4.33.1**, native SDK metadata **3**, **emery** only.
- Both final live and mock modes compiled successfully. Default source has
  `PEBBLE_MOCK_DATA=0`, `PEBBLE_USE_LEGACY_UI=0`.
- Live RAM footprint: **35,875 / 131,072 bytes**, available heap **95,197 bytes**.
  The retained mock mode also compiled successfully.
  Resource pack: **4,342 bytes** in both.
- Final bundle is `build/pebble-flight.pbw`. Verified its embedded display name
  **Pebble Flight** and original UUID `e6fc90f6-5f5a-4f34-a18e-76f13a5b2327`.
  Verified a known mock booking fixture is absent from the normal bundle; fixture
  C data is compiled only in mock mode.
- No C compiler warnings. The SDK linker emits its ELF LOAD-segment RWX warning.
- npm dependencies installed successfully; pinned Moment Timezone **0.6.5** and
  transitive Moment **2.31.0** are recorded in `package-lock.json`. SDK build
  bundled the phone JS successfully; the companion loaded in the actual emulator.

## Automated suites

`sh scripts/test.sh` passed all six suites:

1. Existing flight transformation, delay thresholds, adaptive refresh and
   automatic seat-position rules.
2. Existing multi-flight settings, booking/seat storage and manual overrides.
3. Mock startup isolation, live startup gating, duplicate-mode cached resync
   without restarting the service, and explicit phone settings.
4. Native mock scenarios, progress semantics, bounded four-page navigation,
   list selection/scroll restoration, missing fields and booking positions.
5. New production adapter and companion tests: scheduled/current airport-local
   times; overnight dates; DST fall-back; fractional UTC offset; unknown timezone;
   explicit operational state/progress; gate versus runway basis; null versus
   unavailable fields; legacy cached-route fallback; separate duplicate entries;
   serialized/coalesced snapshots; synthetic HTTP success/error callbacks; an
   out-of-order response after switching flights; API-key exclusion from payloads;
   phone settings without starting a lookup; landed-flight retention; explicit
   post-landing refresh; clearing all flights into an empty list.
6. New native store tests: atomic multi-flight snapshots, owned string lifetimes,
   stale/out-of-order generations, invalid counts/statuses/indices and empty state.

The C tests use `-Wall -Wextra -Werror`. `git diff --check` passed.

## Native emulator

Tested the compiled application in the **actual local Emery QEMU emulator** at
**200×228**, using `pebble emu-button`, `pebble send-app-message` and
`pebble screenshot`.

### Approved design regression

- Ran all six mock flights through All Flights, Overview, Departure, Arrival and
  Booking, plus the data note and empty list.
- **90 framebuffer comparisons passed** for page limits, reverse navigation,
  Select/note actions, Back restoring list position, empty toggling and unchanged
  screen content.
- An additional full-image comparison verified **all 32 individual mock screen
  framebuffers match approved commit `fc135ba` pixel for pixel**, including the
  right-hand navigation strip. The live integration does not restyle the design.
- The final build also retains the fixed mock mode, with no phone/data startup.
  The frozen preview branch and its distinct app UUID remain unchanged.

### Production data path

- Started the live-default app and observed its real empty setup state; no saved
  flights or credentials were configured in this emulator.
- `scripts/review-live-ui.py` generated synthetic provider records through the
  real JS adapter and sent them over actual AppMessage into the production C
  store and renderer. These are **fixture-driven native tests**, not live airline
  status checks. No provider requests or settings writes were made by this review.
- **Seven production framebuffer comparisons passed**: Overview/Booking limits,
  reverse order, data-note closing and selection restoration, including the third
  saved flight. This also verifies the list with one to three flights avoids the
  old mock-only scrollbar division-by-zero condition.
- Captured/inspected all four live detail screens, list, update/error notes,
  actions menu, retained Aircraft view, saved-data/unknown-progress state and
  clearing back to an empty list. Text/seat highlights fit the native display.
- Rechecked the message path after the final scheduled-timezone fields were added.
  Separate focused synthetic cases inspect a DST-change departure and a delayed
  departure crossing local midnight. These verify separate schedule/current zone
  and date labeling in the native airport page.
- The CLI Bluetooth toggle did not produce a sustained app-disconnected state
  across screenshot connections in this emulator. Physical disconnect/reconnect
  remains unverified. The JS duplicate-mode recovery path is unit-tested; the
  native app retries its mode handshake while waiting for a phone snapshot.
- Default bundle was restored to live mode. No real booking information, API key,
  paid flight lookup, physical watch install or appstore release was involved.

Evidence: [production contact sheet](live-ui/contact-sheet.png),
[production navigation results](live-ui/results.json),
[approved mock screenshots](screenshots/contact-sheet.png),
[mock navigation results](screenshots/navigation-results.json).
Individual PNGs are unmodified native screenshots. Contact sheets only arrange
those PNGs for review; they are not simulated watch layouts.

## Remaining checks for CloudPebble and a physical watch

CloudPebble's authenticated import/build/emulator and physical-watch migration
were not exercised in this session. A real FlightAware lookup also remains
account-dependent and unverified here.

1. Import/pull **main**, using **Pebble C SDK / SDK 3 / emery** and the npm
   dependency from `package.json`. Compile and run **Pebble Flight**.
2. Verify existing phone settings survive the update, or enter flights/key for a
   fresh installation. The UUID and existing storage keys are unchanged; a
   physical migration was not tested.
3. Validate one real flight against the provider: correct departure-date match,
   both airports, separate gate schedule/estimate/actual, operational state,
   progress, terminal/gate/baggage availability and update timestamp. Confirm
   account/service access on the phone's JS runtime.
4. Check all four screens and buttons, hold-Select actions, cached/offline/error
   behavior, aircraft history and manual/automatic seat positions on that runtime.
5. To evaluate without credentials, use the frozen preview branch, or enable
   `PEBBLE_MOCK_DATA=1` in an emulator build and follow the six-scenario table.

Source promotion does not publish or submit an appstore release. No backend,
FlightAware account, API key or phone-settings site deployment was created.
