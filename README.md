# Jetebble

**Overview mockup — unapproved, local branch `codex/overview-airline-mockup`.**
This branch centers the airline logo inside the circle, reduces the percentage,
aligns an aircraft badge with the progress stroke and adds all 17 supplied airline
logos. `main` remains unchanged. See [the mockup and offline preview](docs/OVERVIEW_MOCKUP.md).
The release notes below describe the existing production version.

**Version 0.7.4** renames Pebble Flight to **Jetebble** and uses the approved green native design as its
current interface. It runs on **Pebble Time 2 / emery (200×228)** using the
**Pebble C SDK**, with PebbleKit JS providing flight data and phone settings.

The default build uses the existing **live FlightAware integration**. Mock mode
is off. The original app UUID is restored so this builds as an update to Pebble
Flight, with the same phone-storage keys for saved flights, API key, booking
codes, seat positions and cached data.

Version 0.7.4 adds a native next-flight launcher subtitle and the supplied color
logos for American, Delta, Frontier, Southwest and United on Overview. Existing
flight data, booking functionality and the other detail screens are retained.
See [Jetebble branding and launcher behavior](docs/JETEBBLE.md).

Version 0.7.3 uses the supplied plane-stabilizer artwork as its launcher icon.
The original SVGs and all three PDC conversions are in `resources/launcher-icons/`.

Version 0.7.2 enlarges the Overview progress circle from 98 to 116 px so the
airplane clears the flight number. Other screen content and behavior are unchanged.

Version 0.7.1 fixes native-data initialization after upgrading from the old UI,
immediately retries after saving a different AeroAPI key, and preserves snapshot
ordering when the phone companion restarts. Existing refresh intervals resume
after the first successful native-data lookup. The design and icons are unchanged.
See [the sync-fix validation](docs/LIVE_SYNC_FIX.md) for the reproduced failure
and companion-to-watch test coverage.

## CloudPebble

1. Sign in to [CloudPebble](https://cloudpebble.repebble.com/).
2. Import `https://github.com/kvnsr1/pebble-flight`, branch **`main`**, into a
   new project. This is recommended if your existing project is Flight Preview.
   For an existing production project linked to GitHub, pull `main` and verify
   its Settings/UUID and message keys match `package.json` before rebuilding.
3. Choose **Pebble C SDK / native**, **SDK 3** metadata, target **emery / Pebble
   Time 2**. Keep the imported UUID, message keys and npm dependency.
4. Compile in **Build & Run**, then run on **Emulator → emery** or install on
   your watch through the Pebble phone app's Developer Connection.
5. In the phone app's Jetebble settings, use your existing flights and
   AeroAPI key, or add them if this is a new installation.

[Open the GitHub import dialog](https://cloudpebble.repebble.com/ide/import/github/kvnsr1/pebble-flight)
(set branch `main`). SDK metadata `3` is the native API generation; the validated
local toolchain is **SDK 4.33.1 / Pebble Tool 5.0.40**.

The launcher and companion-app name is **Jetebble**, UUID
`e6fc90f6-5f5a-4f34-a18e-76f13a5b2327`. No appstore release is published by
building, importing, or updating this repository.

## Interface and buttons

All Flights lists all configured flights, with route, departure date, operational
status and a clear selected row. The existing phone configuration supports up to
three flights. Logged landed flights stay until you remove them in settings.

The detail sequence is **Overview → Departure → Arrival → Booking**:

- **Up / Down:** move through the list or detail pages, stopping at either end.
- **Select on a flight:** open Overview and fetch that flight if its update is due.
- **Select on details:** open/close its data note, including update time, next
  refresh, errors, source/freshness and field meanings.
- **Hold Select:** open **Refresh flight / Aircraft details / Close**. Refresh
  bypasses the normal schedule with a 30-second duplicate-request cooldown.
  An explicit refresh can update baggage after landing; automatic terminal-flight
  polling remains off. Aircraft details preserve the prior model, registration,
  unavailable first-flight date and lazy-loaded incoming-leg history.
- **Back on any main detail or data note:** return directly to All Flights,
  preserving selection and scroll. Back from Aircraft returns to the main detail.
- **Back on All Flights:** exit. Phone settings are available even when the watch
  app is not open.

Booking retains the ticket motif, booking reference, seat number and highlighted
window/middle/aisle diagram. Saved seat-position overrides take priority over the
existing automatic A/F, B/E, C/D rule. Booking values are editable in phone
settings; the watch view is read-only. No booking details or API key are sent to
the hosted settings page as a URL query; booking state uses the fragment and the
key stays in phone storage.

## Live data behavior

- Refresh, caching, error handling, account configuration and the existing AeroAPI
  endpoint are retained. Opening another flight selects its exact saved index.
  Pending responses update their own flight, even after selection changes.
- The five-minute heartbeat checks the existing adaptive schedule: beyond seven
  days, no polling; one to seven days, the immediate next flight every six hours;
  final 24 hours, hourly; final six hours, every 15 minutes; departed, every ten
  minutes. Canceled and landed flights stop automatic updates.
- **LIVE** indicates a cached result within the refresh policy; **SAVED** or
  **OFFLINE** indicates saved/old data, and **WAIT** means an update is in progress.
  Select opens exact update/source/error details. A failed request preserves data.
- Times are **24-hour airport-local**, with each endpoint's local date, timezone
  abbreviation and UTC offset. Bundled Moment Timezone/IANA data handles DST
  without depending on `Intl` support. Unknown timezones remain explicitly
  unknown instead of showing UTC as if it were airport-local.
- Scheduled and estimated/actual **gate** times remain separate. A runway time
  is never substituted to manufacture a gate estimate or a gate delay. If an
  estimate/actual crosses to a new local date, the time-row labels show both dates.
- Boarding, departed/taxiing, airborne, landed, delayed, canceled and diverted are
  operational states. Unknown gate, terminal or baggage data is not invented.
  **Unknown** means not reported; **N/A** means unavailable/inapplicable.
- Progress uses FlightAware's explicit `progress_percent`. Upcoming, boarding,
  departed/taxiing and pre-departure delayed flights stay at 0%; landed is 100%;
  canceled has no journey. Missing airborne progress shows **PROGRESS UNKNOWN**.
  It is never inferred from formatted local times.
- Old cached routes and booking details remain usable after upgrade. Fields not
  present in the old cache stay unknown until a successful fresh lookup.

The existing direct AeroAPI/key-on-phone setup remains a personal integration.
No new backend is deployed here; the original integration notes are preserved in
[docs/LIVE_APP.md](docs/LIVE_APP.md). A live lookup requires a valid account and
working service access. Native/adapter tests do not verify account access or
provider coverage for a real flight.

## Mock mode and approved prototype

The reviewed prototype remains frozen on **`codex/native-flight-ui-prototype`**
(commit `fc135ba`) with its separate **Flight Preview** UUID. You can import that
branch into CloudPebble to revisit the original credential-free prototype.

To test the same approved design in the current source, set this one flag in
`src/c/prototype_config.h`, then rebuild and run **in the emulator**:

```c
#define PEBBLE_MOCK_DATA 1
```

Mock mode does not open AppMessage, start polling, or read phone settings on app
startup. Keep `PEBBLE_USE_LEGACY_UI` at `0`. Hold Select on All Flights toggles the
empty demo; Select on details toggles the note; detail long Select has no action.
These controls match the frozen prototype. Set `PEBBLE_MOCK_DATA` back to **0**
and rebuild for the normal live app. The main app UUID is unchanged by this flag.

| Flight | Fixed synthetic scenario | Booking example |
|---|---|---|
| AA100 / JFK → LHR | Upcoming, unknown estimates/gate, next-day arrival | Q7M2KP / 12A / Window |
| UA206 / SFO → ORD | Boarding, on-time estimates, unknown baggage | R4N8VX / 18B / Middle |
| BA284 / SFO → LHR | In flight at 62%; departure late, arrival early | K8T3WL / 21C / Aisle |
| DL418 / LAX → JFK | Landed at 100%, actual early/late times | V2H6FQ / 8C / saved Window override |
| AF83 / SFO → CDG | Delayed 75 minutes, long name, missing gate/baggage | DEMO12345678 / Unknown seat |
| EK202 / JFK → DXB | Canceled; schedules retained, current times N/A | N/A booking/seat |

These are independent frozen examples, not airline operational claims.
[Native prototype screenshots](docs/screenshots/contact-sheet.png) and the
[original validation record](docs/PROTOTYPE_VALIDATION.md) remain available.

## Local build and validation

```sh
npm ci
pebble build
pebble install --emulator emery
sh scripts/test.sh
```

Output: `build/pebble-flight.pbw`. To reuse the existing private Mac tools:

```sh
export PEBBLE_TOOLS_DIR="$HOME/Developer/pebble-flight/.tools"
sh scripts/build.sh
sh scripts/emulator.sh
```

`scripts/review-prototype.py` exercises all mock scenarios and compares the
native framebuffer against the approved screens. Enable mock mode and rebuild
before running it. `scripts/review-live-ui.py` checks the production message path
using synthetic provider data in an emulator with no saved flights. It does not
configure credentials or make provider calls. Both need Pillow and `pebble` on PATH.

See [current validation](docs/VALIDATION.md) for exact test coverage and remaining
CloudPebble/physical-watch checks. The source directory named `prototype/` now
contains the shared approved renderer: `live_data.*` and `live_store.*` adapt
production AppMessages to `flight_model.h`; `mock_flights.c` is compiled only in
mock mode. The legacy five-page UI remains an optional rollback behind
`PEBBLE_USE_LEGACY_UI=1`; it is not the default.

## References

[CloudPebble](https://developer.repebble.com/sdk/cloud/),
[Pebble SDK](https://developer.repebble.com/sdk/),
[CLI/emulator](https://developer.repebble.com/guides/tools-and-resources/pebble-tool/),
[Pebble iconography](https://github.com/pebble-dev/iconography),
[Moment Timezone](https://momentjs.com/timezone/docs/).
The original PebbleOS flight SVG, license and translation notice are retained in
`resources/pebble-iconography/`. Moment and Moment Timezone MIT licenses are
retained in `docs/licenses/`.
