# Jetebble 0.7.6 — future-flight lookup and unknown progress

Reported on 20 October 2026: WN3156 departing 22 October showed only its saved
number, `Updated Never` and `Flight service error (400)`.

## Request correction

The previous query padded the selected airport-local date in UTC: an October 22
lookup requested October 21 00:00 UTC through October 23 23:59:59 UTC. This can
exceed `/flights/{ident}`'s maximum future date of two days, which is a rejected
parameter even when the requested flight itself is in range. The limits and
`ident_type` behavior were checked against the
[official AeroAPI specification](https://www.flightaware.com/commercial/aeroapi/resources/aeroapi-openapi.yml).

The query now retains timezone padding and intersects it with the supported
window (10 days back / 2 days forward), with a one-minute clock margin. The
returned flight must still match the **departure airport's local date**.
Invalid dates and dates wholly outside the live window show an explanation
without issuing an invalid request. The request removes spaces from the flight
number and sets `ident_type=designator`; saved identities and cache keys stay
unchanged.

An empty future result remains unknown and asks the user to retry closer to
departure. This change does not use the separate schedules service, guess a
route, or manufacture gate/status/timing details before they are reported.
The existing request count limit and duplicate-tap cooldown remain in place.

## Overview

Replace the two-line `PROGRESS / UNKNOWN` heading with **NA**, centered inside the
circle above the airline logo. Known progress percentages, the aircraft marker,
canceled-flight presentation and other pages retain their layout.

## Validation

- All ten automated suites pass, including the motion suite from the newer
  `e824ffa` update, which is preserved. The companion integration suite covers an
  Oct 20 request for Oct 22, successful synthetic data delivery, empty future
  results, invalid/out-of-window dates, the historical boundary, space-separated
  flight numbers, cooldown behavior, and an evening departure whose UTC date is
  the following day.
- Fresh native C SDK 3 / Emery build with local SDK 4.33.1 and Pebble Tool 5.0.40.
  Resources: 16,487 bytes; RAM footprint: 44,869 bytes; free heap: 86,203 bytes.
  Only the existing SDK ELF LOAD-segment RWX linker warning was reported.
- Default live mode, Jetebble name and existing app UUID remain in place.
- Offline Emery screenshots and physical-button events cover all 18 airline
  examples with the preserved motion code. All 17 known-progress/canceled
  Overviews, All Flights, Departure, Arrival, Booking and restored list selection
  match the earlier native screenshots pixel for pixel. Only the unknown-progress
  label area changes. [Native comparison](future-flight-fix/preview.png) and
  [machine-readable checks](future-flight-fix/results.json) use synthetic data,
  not WN3156's actual itinerary or provider response.
- Direct synthetic-message verification in the local live-mode emulator was
  unavailable: test messages were not acknowledged. The request correction is
  verified by the companion integration harness, not by an authenticated API call.

All automated API responses are synthetic. No account credential was accessed
and WN3156's actual provider response has not been tested. CloudPebble and
physical-watch installation are not exercised locally.

## Install and retry

In the production CloudPebble project, pull `kvnsr1/pebble-flight`, branch `main`,
compile and run on Phone. Leave mock mode disabled. Keep the existing saved key
and flights. Open WN3156; to retry explicitly, hold Select and choose **Refresh
flight** (the normal 30-second duplicate-request cooldown still applies).
If the record is not yet available, Select's information note explains the
lookup result. Data will appear when FlightAware returns a matching record.
