# Pebble Flight

A Pebble Time 2 flight-status app inspired by the glanceable parts of Flighty. It shows route, date, local departure/arrival times, delay severity, and departure/arrival terminal and gate.

## Architecture

- The native C watch app targets `emery` (Pebble Time 2).
- PebbleKit JS runs in the Pebble phone app, calls FlightAware AeroAPI, caches the last successful response, and sends a compact message to the watch.
- The settings page stores up to three flights and one AeroAPI key.

The watch itself does not have internet access, so the connected phone is required for fresh data. Gate and terminal values are shown as `--` when an airline/airport has not published them.

## Mac setup

This project is already set up at `~/Developer/pebble-flight` with private copies of Python 3.13, Node, and the Pebble command-line tool under `.tools/`. To build it without changing your Mac's system configuration:

```sh
cd ~/Developer/pebble-flight
./scripts/build.sh
```

The finished package is `build/pebble-flight.pbw`.

To build and launch it in the Pebble Time 2 emulator:

```sh
./scripts/build.sh
./scripts/emulator.sh
```

For a fresh installation on another Mac, use the steps below.

1. Install Homebrew if needed, then install current Python and Node:

   ```sh
   brew install python@3.13 node uv
   ```

2. Install the current Pebble command-line tool and SDK:

   ```sh
   uv tool install pebble-tool --python 3.13
   pebble sdk install latest
   ```

3. Build the app from this directory:

   ```sh
   pebble build
   ```

4. Run it in the Time 2 emulator:

   ```sh
   pebble install --emulator emery
   ```

   For a physical watch, enable **Dev Connect** in the Pebble mobile app, then use the install command shown by the app. The legacy local-network form is `pebble install --phone PHONE_IP`.

## Connect live data

1. Create a FlightAware AeroAPI v4 Personal account and API key.
2. Publish the `config/` folder as a static HTTPS site (GitHub Pages works).
3. Replace `CONFIG_URL` near the top of `src/pkjs/index.js` with that HTTPS URL.
4. Rebuild and install the app.
5. Open Pebble Flight's settings in the phone app, then enter up to three IATA/ICAO flight identifiers such as `AA100`, their departure dates, and the key.

For personal testing, the key is stored in the Pebble phone app's local storage and sent in AeroAPI's `x-apikey` header. It is never passed to the hosted settings page. Do not ship a public build this way: FlightAware does not support browser-side CORS requests and recommends a backend application server. Put AeroAPI behind a small serverless proxy so users cannot extract or abuse your key. FlightAware's current Personal terms are for personal/academic derivative use; a public consumer app requires the appropriate commercial tier.

## Controls

- **Up / Down:** move between Summary, Departure, and Arrival.
- **Select:** refresh the visible flight.
- **Long-press Select:** switch to the next saved flight.
- **Back:** exit.

While the app is open, it refreshes only the visible flight every 10 minutes. Continuous background alerts are a later phase and should use FlightAware Flight Alerts plus a secure push service rather than aggressive polling.

## Delay rules

Before departure, severity is based on the latest gate-departure estimate versus schedule. Once the flight has departed, it switches to the latest arrival estimate versus schedule.

- On time: no positive delay against the active schedule
- Slight delay: 1–29 minutes
- Delay: 30–60 minutes
- Major delay: more than 60 minutes
- Canceled/diverted flights override delay severity

## Test data mapping

If Node is installed, run:

```sh
node test/flight.test.js
```
