# Supplied scheduled-flight aircraft

The two SVGs are byte-for-byte copies of the user's attachments from
`resources/images/plane_icon/` in their local Developer checkout. The white fill,
black outline and transparent background are preserved. The 80×80 artwork has
motion lines. The originals are retained here; their SHA-256 hashes and the PDC
hashes are recorded in `manifest.json`.

Converted with [HBehrens pdc_tool](https://github.com/HBehrens/pdc_tool), version
0.3.4, before changing the native renderer:

```sh
pdc_tool Scheduled_flight_50x50.svg pdc Scheduled_flight_50x50.pdc
pdc_tool Scheduled_flight_80x80.svg pdc Scheduled_flight_80x80.pdc
```

The 50×50 PDC has 3 commands / 103 bytes; the 80×80 PDC has 8 commands / 188 bytes.
The 50 px conversion has no diagnostics. The 80 px SVG contains one fractional
stroke width (3.2866) that PDC quantizes. The converter's exact diagnostic is:

> Stroke width of 3.2866 cannot be expressed in PDC. Clamped to closed valid color.

SVG and PDC renders made with this tool matched pixel for pixel at source
resolution against black, white and dark-green backgrounds. This verifies the
tool's own rendering paths, not arbitrary SVG renderer equivalence.

Both original-size PDCs are bundled as raw resources. `pebble_plane.c` loads and
scales their vector command coordinates once to a **25×25 on-screen footprint**,
keeping native integer stroke widths at least 1 px. No raster intermediate is
used. This preserves the accepted marker position and footprint rather than
covering the Overview's text with a literal 80 px marker.

- **In flight, known progress:** use the 80×80 source with motion lines.
- **Other states with known progress:** use the 50×50 source.
- **Canceled or unknown progress:** preserve the existing no-marker behavior.

The circle centerline, start/sweep angles, percentage, airline logo, flight
number, status row, other screens and controls remain unchanged. The solid white
badge has been removed. The original Pebble GPath fallback remains available if
a resource cannot be allocated; its attribution is in `../pebble-iconography/`.
