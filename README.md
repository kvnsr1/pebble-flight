# Pebble Flight — native UI prototype

Interactive **Pebble C SDK** app for **Pebble Time 2 / emery (200×228)**.
Branch: **`codex/native-flight-ui-prototype`**. Launcher name: **Flight Preview**.
Mock mode is enabled by default. No credentials, phone configuration or flight
service is needed. All examples are synthetic and frozen in October 2026.

The design adapts the supplied reference's green palette, circular progress,
large airport codes and page dots to Pebble system fonts, 64 colors, native
graphics and physical buttons.

## Import, build and run in CloudPebble

1. Sign in at [CloudPebble](https://cloudpebble.repebble.com/).
2. In the project list choose **Import → Import from GitHub**.
3. Repository: `https://github.com/kvnsr1/pebble-flight`.
   Branch: **`codex/native-flight-ui-prototype`**. Enter this branch explicitly.
4. Use **Pebble C SDK / native**, **SDK 3** metadata, target **emery / Pebble
   Time 2** only. Keep the imported UUID and message keys. CloudPebble's default
   new-project type may be JavaScript SDK (Alloy); this project uses native C.
5. Open **Build & Run**, compile, select **Run On → Emulator → emery**, then
   run/install the successful build. Both browser and cloud emulators support
   this credential-free prototype.
6. Use the emulator's physical buttons as below. Do not use Publish.

[Open the GitHub import dialog](https://cloudpebble.repebble.com/ide/import/github/kvnsr1/pebble-flight)
(set its branch field as above). Upload Zip also accepts a standard project ZIP
containing `package.json`, `wscript`, `src/` and `resources/`.

## Buttons

| Screen | Up / Down | Select | Hold Select (0.7s) | Back |
|---|---|---|---|---|
| All Flights | Move selection and scroll; stop at list ends | Open Overview | Toggle empty/filled demo, retaining position | Exit |
| Detail pages | Overview ↔ Departure ↔ Arrival; stop at ends | Toggle data/timezone note | No action | Return directly to All Flights, retaining selection and scroll |
| Data note | Close note and move one detail page, respecting limits | Close note | No action | Return directly to All Flights |
| Empty list | No action | No action | Restore six examples | Exit |

The three right-hand dots show Overview, Departure, Arrival; the active dot is
larger and white. Tiny arrows indicate available detail pages. Native emulator
keyboard mappings: Up=`W`/↑, Down=`X`/↓, Select=`S`/→, Back=`Q`/←. CloudPebble
also provides visible watch buttons; keyboard focus must be on the emulator.

## Inspect the mock scenarios

All six examples appear in All Flights. Press Down to select, Select to open,
Down twice for the airports, Back to return. No rebuild is required. Each is an
independent frozen example rather than a live shared clock.

| Row | Flight / route | Scenario and edge cases |
|---|---|---|
| 1 | AA100 / JFK → LHR | Upcoming; estimates not reported, unknown gate, unavailable baggage; next-day arrival; EDT → BST |
| 2 | UA206 / SFO → ORD | Boarding; on-time estimates; gate F12, baggage unknown; PDT → CDT |
| 3 | BA284 / SFO → LHR | In flight, fixed 62%; actual departure 12m late, estimated arrival 12m early; next day |
| 4 | DL418 / LAX → JFK | Landed, 100%; actual departure 5m early, actual arrival 18m late; baggage B7 |
| 5 | AF83 / SFO → CDG | Delayed 75m; unknown gate, unavailable baggage; long airport name; next-day CEST arrival |
| 6 | EK202 / JFK → DXB | Canceled; schedules retained; current times, terminal, gate and baggage unavailable; EDT → GST |
| Hold Select on list | Empty list | No selectable rows; hold again to restore the prior position |

## Time, status and progress

- **24-hour airport-local time** throughout. Each endpoint shows its own date
  and timezone; airport detail pages include the UTC offset for that date,
  including daylight saving time. Values never depend on the watch timezone.
- Overview labels SCHED, EST or ACTUAL. Airport pages separate scheduled and
  estimated/actual values, with numerical early/late status for that endpoint.
- **Unknown** means not reported; **N/A** means unavailable or inapplicable.
  Missing estimates stay Unknown on airport pages. Overview falls back to the
  explicitly labeled schedule. Cancellation preserves schedules and shows N/A
  for current times. No new estimate is invented.
- The circular track sweeps 270° from lower left to lower right. Upcoming,
  boarding and pre-departure delayed flights stay at 0%, with the plane at the
  start. In-flight progress is explicit model data (62% here). Landed is 100%,
  with the plane at the end. Canceled shows a muted track, X and NO JOURNEY,
  with no plane or completed progress. The model clamps airborne values to 0–99.
- Every status has text and a clock, direction arrow, checkmark or X. Color
  reinforces the label. Percentages remain fixed during review and are never
  calculated from timezone-formatted strings.

## Existing architecture and isolation

The original project uses `projectType: native`, `sdkVersion: "3"`,
`enableMultiJS: true`, and `targetPlatforms: ["emery"]`. These settings and the
recursive native C/PebbleKit JS `wscript` build remain. Validation used SDK
**4.33.1**; SDK 3 metadata denotes the native app API generation, not the
installed toolchain version.

| File | Responsibility |
|---|---|
| `src/c/prototype_config.h` | Single compile-time mock/live switch |
| `src/c/prototype/flight_model.h` | Provider-neutral flight, airport, time and presence model |
| `src/c/prototype/mock_flights.c` | Only fixture source; six deterministic examples |
| `src/c/prototype/navigation.*` | Selection, scroll position, bounded detail navigation |
| `src/c/prototype/prototype_ui.c` | Native Layer/GContext rendering and button handlers |
| `src/c/prototype/pebble_plane.c` | PebbleOS 25px flight icon translated into GPaths |
| `src/c/main.c` | Original five-page live app behind the mode switch |
| `src/pkjs/live.js` | Original phone service, moved from index.js; refresh/cache/settings/aircraft/booking retained |
| `src/pkjs/index.js` | Loads the phone service only after a native LIVE_MODE handshake |

`#define PEBBLE_UI_PROTOTYPE 1` is checked in. It runs entirely on the watch,
without AppMessage, timers, persistent writes, credentials or services. The
phone entry point sends only a mode query; it does not load the production
module or access storage without a native live-mode reply. No second JS switch
is required. Set the C switch to **0** and rebuild to use the original live app
and controls; its instructions are preserved in [docs/LIVE_APP.md](docs/LIVE_APP.md).
Mode 0 is the legacy integration fallback; the new UI still needs a production
adapter after approval.

This branch uses its own UUID `e4baf632-a764-46ec-8262-e90610be4a85` and display
name. The published app's UUID `e6fc90f6-5f5a-4f34-a18e-76f13a5b2327` is retained
only in documentation, so installing Flight Preview does not replace it.

Repository inspection found no GitHub Actions or other checked-in deployment
workflow. The existing scripts build locally and install in Emery. The existing
phone settings site at `https://kvnsr1.github.io/pebble-flight/config/` is unchanged;
no Pages deployment or appstore publication is part of this prototype.

The original phone model carries one current time per endpoint, delay-severity
status, terminal/gate, aircraft, booking and seat data for up to three saved
flights. After design approval, add a production adapter to populate this UI's
neutral model with airport names, separate scheduled/estimated/actual times,
local dates/offsets, operational state, explicit progress and field presence.
The old payload does not supply baggage, airport names or progress. Keep those
Unknown/N/A until provided, keep fixtures separate, and retain existing
refresh, cache and error behavior. The prototype list currently represents all
six mock logs; the existing live service's three-flight limit is preserved.

## Local build and tests

With `pebble`, the SDK, Node and a C compiler installed:

```sh
git switch codex/native-flight-ui-prototype
pebble build
pebble install --emulator emery
sh scripts/test.sh
python3 scripts/review-prototype.py
```

The last command requires Pillow and reinstalls Flight Preview in the emulator,
drives physical-button events, saves screenshots and compares framebuffers for
navigation boundaries and selection/scroll restoration.

The Mac scripts also accept the existing private tool installation:

```sh
export PEBBLE_TOOLS_DIR="$HOME/Developer/pebble-flight/.tools"
sh scripts/build.sh
sh scripts/emulator.sh
```

Output: `build/pebble-flight.pbw`. See [validation results](docs/VALIDATION.md) and
[native screenshots](docs/screenshots/contact-sheet.png).

## Documentation consulted

- [SDK](https://developer.repebble.com/sdk/)
- [CloudPebble workflow](https://developer.repebble.com/sdk/cloud/)
- [CLI and emulator](https://developer.repebble.com/guides/tools-and-resources/pebble-tool/)
- [Hardware platforms](https://developer.repebble.com/guides/tools-and-resources/hardware-information/)
- [Interaction patterns](https://developer.repebble.com/guides/design-and-interaction/recommended/)
- [Drawing primitives](https://developer.repebble.com/docs/c/Graphics/Drawing_Primitives/)
- [Pebble iconography](https://github.com/pebble-dev/iconography/tree/master)
- [Icon style guide](https://github.com/pebble-dev/iconography/blob/master/STYLE-GUIDE.md)

The original PebbleOS SVG and Apache-2.0 license are retained in
`resources/pebble-iconography/`, with the GPath translation noted in NOTICE.md.
