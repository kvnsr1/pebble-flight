# Jetebble motion prototype

Based on Jetebble **0.7.5**, GitHub `main` commit
`fba3b9bbfaf761bee3ad7d5175103f211ff2d525`. Work is isolated on the local
`codex/motion-prototype` branch. No remote branch, production settings, or main
checkout is changed by building or reviewing this prototype.

## Try it

The offline review app is **Jetebble Motion**, UUID
`b8cf4ac3-4e1a-4e92-9c28-99631fab925c`, targeting Emery / Pebble Time 2,
200x228, native Pebble C SDK with SDK metadata `3`. It can coexist with Jetebble.

- Native bundle: `build/jetebble-motion-prototype.pbw`.
- CloudPebble source: `build/jetebble-motion-prototype-source.zip`. Import into
  a **new** project, retain its separate UUID, and compile for Emery.
- Up/Down selects a flight or moves Overview -> Departure -> Arrival -> Booking.
- Select opens a flight or toggles its data note.
- Back returns directly to the preserved list selection and scroll position.
- Hold Select on the list toggles the empty-state demonstration.
- Hold Select on a detail simulates refresh: WAIT and a separate activity mark,
  followed by a synthetic gate update. The airborne BA284 example also alternates
  62% and 70% so the progress glide can be inspected. This is explicitly mock data.

The six independent scenarios cover upcoming, boarding, airborne, landed, delayed,
and canceled flights, overnight airport-local dates, absent fields, and long
booking/airport values. They are frozen examples, not current flight information.
The review app has an inert phone companion and no configuration capability;
it does not access credentials, saved phone settings, or a flight provider.

## Motion design

| Interaction | Motion | Duration |
|---|---|---|
| List selection/scroll | Selection surface glides; rows scroll as a group beneath fixed chrome | 110 ms |
| Open/Back | Selected green row expands/contracts; Overview shares the flight-number anchor | 230 ms |
| Detail paging | Whole cards move vertically; one 3 px overshoot and settle; fixed indicator rail | 190 ms |
| Data note/Aircraft | Short wipe with an 8 px lift | 150 ms |
| Airplane entrance | One 2 px lift and settle inside the opening transition | 140 ms |
| Confirmed progress update | Arc and marker glide between received endpoints; number shows new value immediately | 220 ms |
| Changed data | Small accent marker beside changed fields, removed with a one-shot timer | 650 ms |
| Pending refresh | Three-dot activity in the navigation gutter, separate from journey progress | 120 ms steps |

New input finishes the prior transition and starts the requested action immediately;
there is no animation queue. Content returns to the approved resting geometry.
Arrival from another page never restarts progress from zero. Missing progress and
canceled journeys stay explicit. Refresh failures keep the previous flight data,
stop activity even if a loading flag is stuck, and show SAVED rather than WAIT.
All motion uses integer positions and the existing native artwork/palette.

The implementation uses public SDK `Animation` callbacks and clipped native layers,
without full-screen framebuffer copies or firmware-only APIs. The normal source
retains live mode and its production UUID; the packaging script changes identity,
companion and mock flags only in a separate build directory. `PEBBLE_MOTION_DEMO`
is off in the normal source. The optional Booking micro-animation was deferred:
the card transition already supplies movement while booking values remain legible.

## Official references used

- [Pebble SDK](https://developer.repebble.com/sdk/): native platform and SDK metadata.
- [Core Experience Design](https://developer.repebble.com/guides/design-and-interaction/core-experience/):
  quick feedback, coherent cards, and an icon/title connecting related views.
- [Recommended Guidelines and Patterns](https://developer.repebble.com/guides/design-and-interaction/recommended/):
  familiar buttons, preserved navigation state, readable data and restrained feedback.
- [SDK animations](https://developer.repebble.com/guides/graphics-and-animations/animations/):
  public custom Animation implementations, lifecycle callbacks, and event-loop timers.
- [Pebble iconography](https://github.com/pebble-dev/iconography/tree/master) and
  [style guide](https://github.com/pebble-dev/iconography/blob/master/STYLE-GUIDE.md):
  existing filled PDC/GPath icons, pixel alignment, limited palette and original angles.
- [PebbleOS Weather](https://github.com/coredevices/PebbleOS/tree/d4c2b2902997db698f2a28c07f5953d6e9b96652/fw/apps/system/weather):
  coordinated movement, small overshoots, reverse transitions, and idle stillness.
  Jetebble uses its own small interpolation functions; firmware internals were not imported.

## Build and review

```sh
export PEBBLE_TOOLS_DIR=/path/to/installed/pebble/tools
python3 scripts/build-motion-prototype.py
```

This builds a PBW and a source ZIP only; it does not install, push or publish.
The normal `scripts/build.sh` continues to compile the live application.

For native emulator review, install from `build/motion-prototype/pebble-flight`,
then run `scripts/review-motion.py` using the SDK Python with Pillow/pebble-tool.
The script captures actual QEMU frames, compensates for the emulator's uniform
backlight dimming, and applies the SDK's standard colour correction. GIF timings
come from capture timestamps. Frame strips, settled PNGs, GIFs and results are
under `docs/motion/`.

`python3 scripts/build-motion-prototype.py --live-test` makes a separate internal
message-path test bundle with an inert companion. Install it only in the emulator
and run `scripts/review-motion-live.py` to send synthetic snapshots through the
real native AppMessage adapter. Restore the offline prototype afterward.

Validation results are recorded in `docs/motion/results.json` and
`docs/motion/live-results.json`. The existing JS/native suites and the added
bounded-motion geometry tests run with `sh scripts/test.sh`. Runtime checks cover
interrupted navigation, exact selection/scroll restoration, operational states,
progress updates, changed gates, refresh errors, unknown progress, and removal
during a transition. Physical-watch timing remains a follow-up check.

Final validation with SDK 4.33.1 passed **10 automated suites, 28 offline emulator
checks and 11 live-path emulator checks**. Offline, message-path and normal live
builds all compiled. The SDK reports 17,801 bytes RAM for the offline review app,
44,869 bytes for the normal live app, and 16,487 bytes of resources in either.
The only build warning is the existing SDK linker LOAD-segment RWX warning.
No provider request or physical-watch installation was performed.
