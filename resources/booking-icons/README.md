# Booking detail icons

Six user-supplied SVGs, preserved without edits, converted with
[HBehrens/pdc_tool](https://github.com/HBehrens/pdc_tool) **v0.3.4**.
Each icon has separate 25, 50 and 80 px artwork; these are not scaled copies.
The PDC files retain their source dimensions and transparent backgrounds.

Recreate all six PDCs from the project root:

```sh
for svg in resources/booking-icons/*.svg; do
  /path/to/pdc_tool "$svg" pdc "${svg%.svg}.pdc"
done
```

The official macOS Apple Silicon ZIP used for conversion has SHA-256
`3ffed0d3c008d0ce63db9ae94694a2535acd4f9088755475ae7940688fde84a0`.
The converter is a development tool, not a dependency of CloudPebble or the app.

Only `Boarding_pass_25px.pdc` and `Plane_bench_window_80px.pdc` are bundled as
native raw resources, at their original size. All six conversions are retained
here for reuse. Only the two image regions on the fourth detail page use them.

`booking_icons.c` draws the supplied artwork with Pebble's GDrawCommand API.
The seat image's backrests (paths 5–7), headrests (8–10), and cushions (21–23)
keep their original geometry. At draw time, the selected seat receives the
existing Malachite accent, white outline and check mark. Other seats retain the
SVG's black/white artwork. The existing seat-position rules determine which
seat is highlighted; unknown and unavailable positions highlight none.
All text, field values, labels, page layout and button behavior are unchanged.
