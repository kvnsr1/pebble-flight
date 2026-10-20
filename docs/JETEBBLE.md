# Jetebble 0.7.4

The watch launcher, companion app metadata and hosted settings heading now say
**Jetebble**. The original UUID and all existing phone-storage keys are retained,
so this is an update of the existing installation. The repository name, npm
package name, PBW filename and settings URL remain `pebble-flight` for compatibility.

## Launcher next flight

The phone builds a launcher plan from the existing saved flights and cached
provider responses. A separate `GLANCE_PLAN` AppMessage delivers it to the native
watch app, which validates it and calls `app_glance_reload` / `app_glance_add_slice`.
No additional flight API requests, credentials, booking codes or seats are involved.
The unsupported `Pebble.appGlanceReload` phone API is not used.

- Flights are ordered by cached estimated/scheduled departure UTC, independently
  of the selected flight. A departed flight remains first until its cached arrival
  boundary. Landed and canceled records are excluded.
- The subtitle contains the flight number, route, reported status and labeled
  scheduled/estimated/actual time with airport-local timezone and date. Airborne
  or departed records show arrival information. Unknown times/zones are explicit.
- A pre-departure slice expires at its cached estimated/scheduled departure. A
  subsequent slice says **Open for flight status**, rather than declaring that
  departure occurred. The flight expires at its last known scheduled/estimated
  arrival, allowing the next cached flight to appear automatically while closed.
  These boundaries are schedule transitions, not fresh airline updates.
- When no arrival is known, a conservative date/time expiry is used; the expiry
  itself is never displayed as a flight time. Date-only flights have no invented
  departure time and use a noon-UTC ordering placeholder internally. Their ordering
  relative to timed flights is approximate until the provider supplies a time.
- Up to seven slices cover three configured flights and the final empty state.
  Removing all flights changes the subtitle to **Add flights in phone settings**.
  Completed/expired flights show **No upcoming flights · Open to view all**.
- A successful phone sync updates the native plan. Pebble stores its slices after
  the app closes; reopening Jetebble refreshes from the existing integration and
  schedule. No new background service or polling is introduced.

Pebble's launcher controls marquee animation, including selection, pause and
rewind. Only the highlighted tile scrolls; this is not an animation drawn by the
closed watchapp. Leave Jetebble highlighted to read a long subtitle. Re-select it
to replay the firmware's selection animation.

## Airline artwork

The supplied 25 px color PDCs appear in Overview's top-left header, replacing its
text heading for supported airlines. The progress circle, moving airplane, flight
number, status, times and navigation stay in their existing positions. Unsupported
airlines keep the neutral heading. Other pages are unchanged.

See [artwork provenance and airline-code mapping](../resources/airline-icons/README.md).

## Install / CloudPebble

Pull `kvnsr1/pebble-flight`, branch **main**, then build with **Pebble C SDK**, SDK
metadata **3**, target **Emery / Pebble Time 2**. Run through **Build & Run → Phone**
to replace the installed app, or choose the Emery emulator. Open Jetebble once so
the phone can transfer its cached launcher plan, then exit to the launcher.

The settings page is served by GitHub Pages from `main` at the existing URL.
There is no appstore publication. CloudPebble import, physical companion-app
metadata caching and physical-watch migration are separate checks below.

## Validation

Local SDK 4.33.1 / Pebble Tool 5.0.40 compiled the native live build successfully:
**38,089 bytes RAM**, **92,983 bytes free heap**, **6,970 bytes resources**. The only
linker warning is the existing SDK ELF LOAD-segment RWX warning.
The existing mock mode also compiled successfully in an isolated source copy:
**10,761 bytes RAM**, **120,311 bytes free heap**, with the same resource pack.
The downloadable/default build remains live mode.

All nine automated suites pass, including chronology/expiry, skipped terminal
flights, empty state, airport-local times, missing data, safe wire lengths,
malformed-plan rejection and the existing live-data integration/navigation suites.
Credentials remain absent from watch messages.

Actual local Emery emulator inspection used synthetic AppMessages, without
configuring a provider key or making paid/live flight lookups:

- All five airline PDCs and their five ICAO aliases rendered correctly; an
  unsupported airline retained its neutral heading.
- The header remained intact at progress 0, 25, 40, 45, 50, 75 and 100 percent.
- Departure, Arrival and Booking matched the prior version pixel for pixel with
  identical input. Back restored the same list selection.
- The launcher displayed **Jetebble**, persisted its subtitle after app exit and
  advanced from WN1145 to AA100 at the synthetic arrival expiry while closed.
- Timed captures immediately after highlighting the tile confirmed horizontal
  movement through flight number, route, status and airport-local departure time,
  followed by a rewind. The firmware plays this once per selection. Earlier
  captures taken after the animation had finished showed only its resting ellipsis.
- A final short native test verified expiry to **No upcoming flights**, then
  reopened the real companion-backed empty setup and restored **Add flights in
  phone settings**. The emulator contains no synthetic saved flights or credentials.
- GitHub Pages successfully deployed `main`; an HTTP fetch verified both the
  hosted settings page title and heading say **Jetebble**.

Evidence: [native preview](jetebble/preview.png),
[launcher animation](jetebble/launcher-scroll.gif),
[all airline overviews](jetebble/airline-overviews.png),
[next-flight transition](jetebble/launcher-next-flight.png),
[final empty state](jetebble/launcher-no-upcoming.png),
[machine-readable results](jetebble/results.json).

The local emulator/bridge stalled during an earlier long idle capture and was
restarted without clearing its saved state. The final short marquee and empty
expiry captures completed successfully.

Emulator screenshots and additional animation evidence are in `docs/jetebble/`.
Physical phone/watch installation, CloudPebble's authenticated workflow, API
account access and a real flight update have not been exercised for this version.
After installation verify the companion name/heading, existing settings, one live
flight, launcher subtitle/scrolling and the supported logo on your watch.

References: [native AppGlance guide](https://developer.repebble.com/guides/user-interfaces/appglance-c/),
[current phone-API limitation](https://developer.repebble.com/guides/user-interfaces/appglance-pebblekit-js/),
[PebbleOS launcher implementation](https://github.com/coredevices/PebbleOS/blob/v4.33.2/src/fw/apps/system/launcher/default/app_glance_structured.c).
