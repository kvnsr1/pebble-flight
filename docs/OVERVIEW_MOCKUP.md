# Overview layout mockup

Review branch: `codex/overview-airline-mockup`. No changes have been pushed or
promoted to `main`; the existing 0.7.4 release remains at `0baeee3`.

## Layout

- Restore the neutral **OVERVIEW** header.
- Use the smaller native **Gothic 24 Bold** percentage, followed by the supplied
  **25 px airline logo**, then the existing **Gothic 18 Bold** flight number.
- Keep the circle's 116 px outside diameter. An 8 px stroke and the aircraft marker
  share a 108 px centerline circle, so the airplane sits on the track. The opening
  is slightly wider (250° of progress arc) to keep the end markers clear of the
  centered information and status row.
- Restore the supplied white/black aircraft, with no circular badge. Convert both
  SVGs to PDC: use the **80×80 source with motion lines for in-flight flights**,
  and the **50×50 source for other states with known progress**. Both fit the
  existing **25×25 display footprint**. The accepted centerline placement and
  progress formula are unchanged; no progress is inferred from times. Canceled
  and unknown-progress flights retain the existing no-marker behavior.
- Add the twelve newly supplied airlines, for **17 total**. Assets are unchanged
  25 px color PDCs. Unsupported carriers use a neutral aircraft in the logo slot.
- Preserve the other detail pages, list, launcher, data integration and controls.

Artwork provenance, hashes and IATA/ICAO mapping are in
[`resources/airline-icons/`](../resources/airline-icons/README.md).
Aircraft sources, conversion commands, fractional-stroke diagnostic and native
scaling are in [`resources/progress-plane/`](../resources/progress-plane/README.md).

## Offline interactive preview

Build the isolated preview with the installed SDK:

```sh
PEBBLE_TOOLS_DIR=/path/to/pebble/tools python3 scripts/build-overview-mockup.py
```

The script copies tracked source into `build/overview-mockup/pebble-flight/`,
enables mock mode **only in that copy**, and substitutes the dedicated
`test/fixtures/overview_mock_flights.c` fixture. It does not edit the checkout,
push Git, call a flight service, publish an app or install anything on a watch.

Outputs:

- `build/jetebble-overview-mockup.pbw`: native **Jetebble Mockup** app.
- `build/jetebble-overview-mockup-source.zip`: complete native CloudPebble source.

The preview has its own UUID, `8a2dccb9-2ff6-4439-9aeb-0876a23c833d`, and can coexist
with production Jetebble. It requires no key or live flight service. The ordinary
branch build retains the production UUID/live mode; use the distinct preview
bundle/source ZIP for design review.

In CloudPebble, import the **source ZIP into a new project**, choose **Pebble C SDK**,
SDK metadata **3**, target **Emery / Pebble Time 2**, compile and run in the emulator.
Keep the preview's imported name/UUID and `PEBBLE_MOCK_DATA=1`.

All Flights has **18 synthetic examples**: the 17 supplied airlines, followed by
an unsupported carrier with unknown progress. Every example uses a deliberately
shared BUR/OAK demonstration route, frozen at 19 October 2026. These are not actual
airline schedules, routes, status reports or bookings.

- First: **WN1145**, upcoming, 0%.
- Second: **AA2220**, airborne, 45%.
- Other rows include boarding, landed/100%, delayed/+75 min, canceled, and more
  airborne progress positions. The final **BA284** row shows the neutral fallback
  and **PROGRESS UNKNOWN**.
- **Up/Down:** list selection or bounded Overview → Departure → Arrival → Booking.
- **Select:** open a flight; on details, toggle its data note.
- **Back:** return to the same list selection and scroll position.
- **Hold Select:** toggle the empty-list demonstration.

The normal six-scenario fixture and production flight integration are unchanged.

## Native screenshots and validation

- [Three progress examples](overview-mockup/preview.png).
- [Previous badge / restored aircraft, identical inputs](overview-mockup/before-after.png).
- [All 17 logos and the neutral fallback](overview-mockup/all-airlines.png).
- [Machine-readable checks](overview-mockup/results.json).

Both the ordinary live-mode build and the isolated offline preview compiled with
Pebble Tool **5.0.40 / SDK 4.33.1**, metadata **3**, **emery**. The live build uses
**38,697 bytes RAM / 92,375 bytes free heap**; the preview uses **13,525 / 117,547**.
Both use **16,487 bytes resources**. The only linker diagnostic is the existing
SDK ELF LOAD-segment RWX warning. All **nine automated suites** pass.

Actual local Emery emulator screenshots cover all 17 airlines, the neutral
fallback, upcoming/boarding/airborne/landed/delayed/canceled states and unknown
progress. This marker revision's native samples cover **0, 25, 35, 45, 50, 60, 65, 75, 90,
100 percent**, rather than every integer. All screenshots use synthetic data;
no provider key was configured and no flight-service requests were made.

The completed offline, button-driven preview run verified all 18 examples. The
prior mockup had already been checked against `main` with identical fixtures.
For this marker revision, **All Flights, Departure, Arrival, Booking and restored
selection again matched the previous screenshots pixel for pixel**. Every changed
Overview differs only within the old badge/current aircraft area (27×27 px);
canceled and unknown-progress Overviews are identical. The 17 airline PDC files
are unchanged. The native 50/80 px resource selection and readability were
inspected in the screenshots. No clipping was observed at the sampled positions.

The source ZIP passed its integrity check and was verified to contain mock mode,
the separate preview identity, all 17 raw logo resources, both aircraft PDCs and no duplicate source
files. The PBW's embedded name/UUID were also checked. Physical-watch testing and
CloudPebble's authenticated import/build/run were not exercised in this session.
