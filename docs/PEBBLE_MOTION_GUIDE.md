# Pebble motion resources and build guidelines

Research and project validation recorded **October 20, 2026**. Use this reference
when asking Codex to create Pebble app animations, animated icons, or motion
assets. Recheck tool releases and target SDK capabilities before a new build.

This document distinguishes published SDK guidance, community tools, and
recommendations derived from the Jetebble implementation. It is a reference,
not an override of the user's current instructions or repository instructions.

## 1. Choose the rendering approach

| Need | Starting point | Useful work for Codex |
|---|---|---|
| Moving UI, progress indicators, simple shapes | Native `Animation`, `PropertyAnimation`, custom drawing | Generate interpolation, easing, clipping, and lifecycle code |
| Expressive vector icons or illustrated frame sequences | PDC images or `GDrawCommandSequence` | Generate compatible SVG frames, compile resources, implement playback |
| Pixel art, characters, textured frames | APNG and `GBitmapSequence` | Process raster frames, encode an animation, integrate timed playback |
| Game sprites, tile maps, scrolling scenes | GBC Graphics library | Prepare tile sheets, palettes, sprite states, and game animation logic |
| An app already using Alloy/Moddable | Piu Timeline or Poco/PDC examples | Adapt examples within that runtime |

Prefer the smallest approach that preserves the artwork. A moving vector icon
does not necessarily require a whole frame sequence: moving the existing image
and drawing procedural effects can be enough. Vectors are not automatically
cheaper for every scene; measure resource size, rendering cost, and heap use.

Sources: [native animations](https://developer.rebble.io/guides/graphics-and-animations/animations/),
[vector animation tutorial](https://developer.rebble.io/tutorials/advanced/vector-animations/),
[animated images](https://developer.rebble.io/guides/app-resources/animated-images/),
[GBC Graphics](https://github.com/HarrisonAllen/pebble-gbc-graphics), and
[Moddable examples](https://github.com/Moddable-OpenSource/pebble-examples).

## 2. Primary documentation

| Resource | What to use it for |
|---|---|
| [Animations](https://developer.rebble.io/guides/graphics-and-animations/animations/) | Property and custom animations, easing, timers, sequential and simultaneous composition |
| [Vector Animations](https://developer.rebble.io/tutorials/advanced/vector-animations/) | PDC image/sequence resources and native drawing |
| [Converting SVG to PDC](https://developer.rebble.io/guides/app-resources/converting-svg-to-pdc/) | SVG preparation, ungrouping, conversion, coordinate warnings |
| [PDC file format](https://developer.rebble.io/guides/app-resources/pdc-format/) | Binary image/sequence structure when debugging converters |
| [Animated Images](https://developer.rebble.io/guides/app-resources/animated-images/) | APNG resource setup, frame timing, reusable bitmap, cleanup |
| [Core Experience Design](https://developer.rebble.io/guides/design-and-interaction/core-experience/) | Fast motion that draws attention to changing information |
| [Recommended Guidelines and Patterns](https://developer.rebble.io/guides/design-and-interaction/recommended/) | Familiar controls, readable hierarchy, clear visual states |
| [Conserving Battery Life](https://developer.rebble.io/guides/best-practices/conserving-battery-life/) | Animation frequency, timers, sensor wakeups, reduced motion |
| [App Assets](https://developer.rebble.io/guides/app-resources/app-assets/) | Existing graphics and asset references |
| [Pebble SDK](https://developer.repebble.com/sdk/) | Current toolchain and supported development paths |
| [PebbleOS PDC reference](https://developer.repebble.com/pebbleos/docs/reference/formats/pdc/) | Firmware format and generator locations for deeper investigation |

The preserved guides include legacy toolchain instructions. Use the installed
SDK headers and a successful target build to confirm exact enums, function
signatures, resource formats, and availability. Do not mix native C, Pebble.js,
Rocky.js, and Alloy APIs merely because they all use JavaScript or Pebble names.

## 3. Asset tools and conversion pipelines

### Native procedural motion

Use `Animation` for elapsed progress and custom drawing for geometry. Use
`PropertyAnimation` when the motion is naturally a layer/property transition.
Sequence animations run components in order; spawn animations run them together.
Read the ownership rules before reusing component animations.

Keep timing separate from drawing, clamp interpolation, and define exact initial
and final positions. Prefer elapsed-time progress over assuming every scheduled
frame will render. Use `AppTimer` when a resource supplies frame delays or the
effect is deliberately stepped. Timer callbacks must explicitly reschedule.

Reference: [native animation guide](https://developer.rebble.io/guides/graphics-and-animations/animations/).

### SVG to PDC

- [HBehrens pdc_tool](https://github.com/HBehrens/pdc_tool) and
  [releases](https://github.com/HBehrens/pdc_tool/releases): CLI conversion and
  Pebble-style PNG preview. Its README can lag released capabilities; inspect
  the installed version and command help. Jetebble's existing plane conversions
  are recorded as **0.3.4** in [the asset notes](../resources/progress-plane/README.md).
- [Web viewer](https://pdc-tool.heikobehrens.com/): quick visual inspection.
- [Inkscape PDC plugins](https://github.com/hellcp/inkscape-pdc-exporter): preview
  SVG artwork within Inkscape; depends on a matching `pdc_tool` binary.
- [cards-example](https://github.com/pebble-examples/cards-example): original
  conversion scripts and a complete native PDC app.
- [svg2pdc-rs](https://github.com/FlyinPancake/svg2pdc-rs): additional Rust
  converter identified in the resource directory; not evaluated in this project.

Commands used by this project's existing plane asset pipeline:

```sh
pdc_tool Scheduled_flight_50x50.svg pdc Scheduled_flight_50x50.pdc
pdc_tool Scheduled_flight_50x50.pdc png plane-preview.png
```

The documented legacy sequence pipeline is:

```sh
python svg2pdc.py --sequence frames/
```

That script is documented for Python 2.x. Inspect its dependencies and pin an
isolated compatible environment rather than installing arbitrary modern versions
into a system Python. Confirm frame ordering, duration, and output naming. The
tutorial's output is a sequence stored in a `.pdc` file; a `.pdcs` filename alone
does not establish converter or runtime compatibility.

Keep SVG inputs simple and render-test them. The legacy converter accepts a
limited element set, and its guide recommends removing groupings and saving
Plain SVG. Browser effects, fonts, filters, masks, and animated SVG markup are
not a guaranteed portable input. Convert text to supported geometry when
needed, simplify transforms, and inspect quantization warnings. Capabilities
differ by converter version.

References: [SVG conversion](https://developer.rebble.io/guides/app-resources/converting-svg-to-pdc/)
and [sequence conversion](https://developer.rebble.io/tutorials/advanced/vector-animations/).

### Raster frames to APNG

[apngasm](https://github.com/apngasm/apngasm) assembles/disassembles APNG files.
The [Pebble animated-image guide](https://developer.rebble.io/guides/app-resources/animated-images/)
documents `GBitmapSequence`; GIF is not directly supported by that playback API.
Its documented conversion is:

```sh
gif2apng -z0 animation.gif
```

For the native pipeline in that guide, use a `.png` filename and declare the
APNG as a `raw` resource:

```json
{"type":"raw","name":"ANIMATION","file":"images/animation.png"}
```

Reuse the destination `GBitmap`, advance using the returned frame delay, mark
the display layer dirty, and stop scheduling at completion. Destroy the decoder
and bitmap when finished. Check the target API and actual decoder compatibility
for palette, transparency, dimensions, and memory rather than assuming any APNG
encoder setting will work. Test disposal/blending and looping with real output.

## 4. Visual and motion guidelines

### Published design guidance

Pebble's [core experience guide](https://developer.rebble.io/guides/design-and-interaction/core-experience/)
uses fast animation to emphasize changed information. Keep important data
readable and controls responsive throughout the effect.

The community [Iconography Style Guide](https://github.com/pebble-dev/iconography/blob/master/STYLE-GUIDE.md)
is especially useful for low-resolution vector art:

- Align points and strokes deliberately to pixels or half-pixels.
- Use clean line ratios such as horizontal, vertical, 1:1, 2:1, and 3:1.
- Inspect both antialiased and non-antialiased rendering where relevant.
- Preserve a clear filled focal shape in icon designs.
- Its 25/50/80 px sizing and stroke conventions address notification/timeline
  iconography; they are not mandatory dimensions for every custom animation.

Use the target's actual colors. The
[Pebble Color Picker](https://developer.rebble.io/guides/tools-and-resources/color-picker/)
and [Aseprite palettes](https://github.com/lavglaab/pebble-aseprite-palettes)
help prepare color art. Add monochrome variants when those watches are in scope.
Review artwork at native resolution; an enlarged preview can hide pixel defects.

### Recommendations from the Jetebble implementation

- Write a short motion specification: trigger, path, duration, easing, overlap,
  loop count, resting state, interrupt behavior, and handoff.
- Use coherent travel direction and a clear cause for each effect. For the
  intro, clouds originate behind the passing plane and disperse at staggered times.
- Let navigation interrupt motion immediately. Avoid queued input or hidden
  navigation triggered by a press intended to skip an intro.
- Treat progress received from a data source separately from decorative motion.
  Do not invent operational progress to make an animation look smoother.
- Define what happens when the window disappears, data changes, memory allocation
  fails, or an effect is interrupted halfway through.
- Allocate reusable art outside per-frame drawing. Keep draw callbacks free of
  network work, sleeps, and repeated expensive resource decoding.
- Name the font explicitly. Native Pebble Gothic, a Pebble logo wordmark, and a
  custom brand typeface are different choices. Jetebble's intro uses
  `FONT_KEY_GOTHIC_28_BOLD`; preserve the exact approved title spelling.

There is no universal FPS or duration recommendation established by this research.
The project's 110–230 ms UI transitions and 3.3-second launch intro are local
design choices, not Pebble standards. Measure on the chosen device.

## 5. Battery and lifecycle

The [battery guide](https://developer.rebble.io/guides/best-practices/conserving-battery-life/)
warns that frequent animation and short timers reduce time asleep. Prefer brief,
event-triggered effects; shorten nonessential motion and consider a reduced or
disabled animation option. Use minute updates when seconds are unnecessary.
Sensor-triggered motion also has a cost; it is not automatically battery-free.

Implementation recommendations: cancel timers and animations at completion,
skip, unload, or loss of visibility; release owned resources and handle allocation
failure. Hiding a layer or suppressing drawing does not by itself stop timer or
animation callbacks. Emulator screenshots establish appearance, not watch battery
life, display response, or hardware timing. Verify those separately on hardware.

## 6. Example code worth inspecting

| Repository | Useful example |
|---|---|
| [pebble-examples/cards-example](https://github.com/pebble-examples/cards-example) | Native PDC artwork and card-based UI |
| [pebble-examples/ks-clock-face](https://github.com/pebble-examples/ks-clock-face) | Expanding clock and sweeping hands |
| [ir33k/prebble](https://github.com/ir33k/prebble) | Vector animation and first-hand SVG/Python conversion problems |
| [HarrisonAllen/pebble-gbc-graphics](https://github.com/HarrisonAllen/pebble-gbc-graphics) | Tile/sprite tooling, starter projects, full games |
| [Moddable-OpenSource/pebble-examples](https://github.com/Moddable-OpenSource/pebble-examples) | `hellopiu-timeline`, PDC movement, rotation, scale, sequence playback |
| [pebble/pebblejs](https://github.com/pebble/pebblejs) | Legacy Pebble.js element animation queues; only use in that framework |

Check licenses before reusing source or artwork. Historical examples can require
SDK/dependency updates. A published repository is evidence of an approach, not
proof it builds unmodified on the current target.

## 7. Forums, wikis, and visual references

- [Rebble development wiki](https://github.com/pebble-dev/wiki/wiki): development
  entry point and links to community tooling.
- [Rebble Developer Forum](https://forum.rebble.io/): Watchapp Development and
  Showcase categories for implementation questions and examples.
- [Pebble forum](https://forum.repebble.com/): Watchapp/Face Developers Corner
  and firmware discussions.
- [PebbleOS issue 663](https://github.com/coredevices/PebbleOS/issues/663): a
  battery-saver API proposal. A feature request is not evidence of a shipped API.
- [Heydays archive](https://old.heydays.no/project/pebble/): original visual style.
- [All Things Pebble: Art & Design](https://allthingspebble.saltedlolly.com/art-design.html):
  a useful tools directory; it does not provide an animation timing/FPS standard.

Forum battery reports are useful leads but not controlled benchmarks. This
research did not find a forum or wiki motion-asset standard stronger than the
SDK guides. Use specific source code or official API documentation to confirm
technical claims before implementing them.

## 8. Reproducible Codex workflow

1. Inspect the project runtime, target models, current artwork, and installed SDK.
2. Choose the rendering method and write the motion specification.
3. Keep editable SVG/PNG inputs, conversion commands, and version notes together.
4. Build the real target. Capture native intermediate frames plus the resting
   state and final app handoff. Generate a GIF and storyboard for review.
5. Exercise interruption, repeat launch, exit/re-entry, and data updates. Verify
   resource cleanup, stable idle state, and existing navigation behavior.
6. Run relevant app checks. When providing an offline demo, isolate its UUID,
   mock data, and inert companion from production identity and settings.
7. Honor the user's current publishing instruction. Keep preview builds local
   when requested; commit/push or release only within the authorized scope.

For Jetebble, start with:

- [startup implementation](../src/c/prototype/startup.c) and
  [integration notes](STARTUP_ANIMATION.md)
- [shared motion helpers](../src/c/prototype/motion.c) and
  [UI motion reference](MOTION_PROTOTYPE.md)
- [offline preview builder](../scripts/build-startup-preview.py) and
  [native emulator review](../scripts/review-startup.py)
- [captured preview](startup/startup.gif) and
  [nine emulator checks](startup/validation.json)

Example prompt for a future Codex session:

> Read docs/PEBBLE_MOTION_GUIDE.md and the repository instructions. Build the
> requested motion for this app's existing runtime and target. Reuse approved
> artwork and native fonts where appropriate. Specify timing, interruption, and
> resting state; generate a reproducible native build and an actual emulator GIF.
> Preserve data semantics and navigation. Follow my current publishing instructions.
