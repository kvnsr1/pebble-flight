# Overview layout mockup

Review branch: `codex/overview-airline-mockup`. No changes have been pushed or
promoted to `main`; the existing 0.7.4 release remains at `0baeee3`.

## Layout

- Restore the neutral **OVERVIEW** header.
- Use the smaller native **Gothic 24 Bold** percentage, followed by the supplied
  **25 px airline logo**, then the existing **Gothic 18 Bold** flight number.
- Keep the circle's 116 px outside diameter. An 8 px stroke and the aircraft badge
  share a 108 px centerline circle, so the airplane sits on the track. The opening
  is slightly wider (250° of progress arc) to keep the end badges clear of the
  centered information and status row.
- Use a white badge with the native Pebble aircraft in dark green. Its position
  follows the same reported progress as before; no progress is inferred from times.
- Add the twelve newly supplied airlines, for **17 total**. Assets are unchanged
  25 px color PDCs. Unsupported carriers use a neutral aircraft in the logo slot.
- Preserve the other detail pages, list, launcher, data integration and controls.

Artwork provenance, hashes and IATA/ICAO mapping are in
[`resources/airline-icons/`](../resources/airline-icons/README.md).

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
- [Current layout / proposed mockup, identical inputs](overview-mockup/before-after.png).
- [All 17 logos and the neutral fallback](overview-mockup/all-airlines.png).
- [Machine-readable checks](overview-mockup/results.json).

Both the ordinary live-mode build and the isolated offline preview compiled with
Pebble Tool **5.0.40 / SDK 4.33.1**, metadata **3**, **emery**. The live build uses
**38,297 bytes RAM / 92,775 bytes free heap**; the preview uses **13,137 / 117,935**.
Both use **16,196 bytes resources**. The only linker diagnostic is the existing
SDK ELF LOAD-segment RWX warning. All **nine automated suites** pass.

Actual local Emery emulator screenshots cover all 17 airlines, the neutral
fallback, upcoming/boarding/airborne/landed/delayed/canceled states and unknown
progress. Native progress samples cover **0, 25, 35, 40, 45, 50, 60, 65, 75, 90,
100 percent**, rather than every integer. All screenshots use synthetic data;
no provider key was configured and no flight-service requests were made.

The SDK transport stalled during a longer AppMessage-driven screenshot sweep.
The completed offline, button-driven preview run verified every airline. A
baseline build used `main`'s original renderer and aircraft helper with identical
preview fixtures, without editing `main`. **All Flights, Departure, Arrival,
Booking and restored selection matched pixel for pixel**. The original five
PDC files still match their previous SHA-256 hashes; all 17 match the attachment.

The source ZIP passed its integrity check and was verified to contain mock mode,
the separate preview identity, all 17 raw logo resources and no duplicate source
files. The PBW's embedded name/UUID were also checked. Physical-watch testing and
CloudPebble's authenticated import/build/run were not exercised in this session.
