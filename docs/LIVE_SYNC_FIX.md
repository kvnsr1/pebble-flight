# Native live-data sync fix — 0.7.1

Validated 17 October 2026. No watch layouts, buttons, C rendering code, icons,
provider endpoint, application UUID or existing settings keys changed.

## Reproduced failures

An upgraded installation can have saved flights, a valid saved key, legacy
display data and a recent refresh timestamp, but no `pebbleFlight.rawCache`.
Version 0.7.0 sends native placeholder rows (`Not updated`, `Updated: Never`),
then skips the lookup because the old refresh schedule is not due. Retained
landed/canceled flights can remain in that state indefinitely. Saving a new key
also reused that schedule and did not necessarily trigger a request.

A reproduction with synthetic storage produced **zero HTTP requests** on both
native startup and saving a replacement key before the fix. The new regression
suite verifies a request occurs and a successful response populates native
status, route, airport-local time and update timestamp.

The actual SDK companion also aborted while formatting a successful response.
Its V8/ICU date formatter caused a fatal runtime error, which JavaScript's
try/catch could not catch. Both phone-local formatting helpers now use native
Date getters and fixed month labels. Airport-local times still use the bundled
IANA timezone data, with their existing DST behavior.

## Changes

- Fetch the first native raw record even when an older refresh schedule or
  terminal state would suppress it. Once populated, the normal schedule resumes.
- Retain a 30-second cooldown for native lookup attempts, including failed
  bootstrap attempts. Duplicate mode hints only resynchronize the cache.
- A changed API key triggers a fresh lookup, bypassing the earlier credential's
  cooldown. Late callbacks from requests using that key cannot replace the new
  result or clear its pending request.
- Persist native snapshot generations so restarting the phone companion cannot
  make an open watch app reject all newer results as out of order.
- Let `index.js` own the ready/mode handshake. Opening settings before ready
  cannot add a second startup callback that switches back to legacy messages.
- Keep cache-processing failures visible in the existing flight information note.

## Validation

- All **seven** suites in `sh scripts/test.sh` pass. The new suite uses the real
  entry point/modules, multiple event listeners, synthetic saved settings and
  HTTP callbacks. It covers recent old refresh state, terminal old state,
  successful cache initialization, resumed scheduling, rejected then replaced
  credentials, pending old-credential replies, blank-key retention, cooldowns,
  storage failures, companion restart ordering and settings-before-ready.
- Successfully built the production **0.7.1** bundle with Tool **5.0.40** / SDK
  **4.33.1**, native SDK metadata 3, Emery. C memory/resources remain unchanged
  from the icon update: RAM footprint 35,679 bytes, heap 95,393 bytes, resources
  5,360 bytes. The usual SDK ELF LOAD-segment RWX linker warning remains.
- Ran an isolated fixture app in the **actual Emery QEMU emulator**, using the
  actual PebbleKit JS runtime, real mode handshake and AppMessage serialization.
  This test initialized old-style refresh state with an empty raw cache, then
  substituted a successful HTTP response from `test/fixtures/provider-flight.json`.
  Unlike earlier raw-frame tests, the watch data was produced by the companion's
  real lookup callback and adapter, without injecting AppMessages from the CLI.
- Captured and inspected the populated list, Overview, Departure, Arrival,
  Booking and information note at 200×228. Route/status/progress/times/facilities
  and a successful UTC update timestamp appeared. The fixture had its own UUID
  (`52b945d8-7791-45b1-a59d-1586f21eaa22`) and a synthetic credential. Its HTTP
  substitute, setup values and UUID are absent from the production bundle.

Evidence: [native companion test screenshots](sync-fix/contact-sheet.png).
The sheet arranges actual emulator screenshots. No real credentials, paid
FlightAware requests, physical-watch installation, authenticated CloudPebble
test or appstore publication was used for this validation.

## Installing the correction

1. Pull `main` into the existing production CloudPebble project.
2. Confirm its version is **0.7.1**, original UUID is
   `e6fc90f6-5f5a-4f34-a18e-76f13a5b2327`, mock mode is off, and message keys
   match the repository. Build and install through **Run On: Phone**.
3. Reopen the watch app and open a saved flight. Its first missing native record
   is fetched without waiting for the previous version's schedule. Other saved
   flights initialize when opened. Existing saved keys/flights remain in place.
4. If needed, hold Select and choose **Refresh flight**. Allow up to the
   15-second network timeout. If a lookup still fails, Select on a detail page
   opens the existing note with the provider/network error. A real account and
   physical-watch check remain necessary to confirm the user's flight service.
