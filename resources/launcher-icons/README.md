# Plane stabilizer launcher artwork

The three supplied `Plane_stabilizer_{25,50,80}px.svg` files are preserved byte
for byte. All three were converted to PDC with
[HBehrens/pdc_tool](https://github.com/HBehrens/pdc_tool) **0.3.4**.
Each PDC has five commands and retains its original canvas dimensions.

To recreate the PDC files from the project root:

```sh
for svg in resources/launcher-icons/*.svg; do
  /path/to/pdc_tool "$svg" pdc "${svg%.svg}.pdc"
done
```

Pebble's launcher uses a **25×25 PNG bitmap** designated by `menuIcon: true`,
as documented in the SDK [image guide](https://developer.repebble.com/guides/app-resources/images/).
The existing `IMAGE_MENU_ICON` resource still points at
`resources/images/menu_icon.png`. That PNG now comes from the 25px PDC, rendered
at native resolution with `pdc_tool`:

```sh
pdc_tool resources/launcher-icons/Plane_stabilizer_25px.pdc png \
  --offset 0,0 --background-color magenta --no-antialiasing \
  --crop 0,0,25,25 /tmp/plane-stabilizer-keyed.png
```

The unused magenta matte is made fully transparent in the final RGBA PNG.
Anti-aliasing is disabled so the matte cannot introduce colored edge pixels;
opaque black/gray/white artwork is preserved. There is no background tile.
`resources/images/menu_icon.svg` mirrors the supplied 25px source.
The standalone PDCs remain available here; they are not unnecessarily bundled
as unused watch resources. CloudPebble builds from the checked-in PNG and needs
no converter installation.
