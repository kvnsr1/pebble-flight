# Stabilizer launcher icon — 0.7.3

Updated 19 October 2026. The app launcher now uses the supplied 25px plane
stabilizer artwork. The application UUID, native SDK/platform settings, flight
screens, in-app airplane, booking icons and data integration are unchanged.

All three SVG sources are preserved in `resources/launcher-icons/`, alongside
the **25×25, 50×50 and 80×80 PDC** conversions created with the requested
HBehrens `pdc_tool` **0.3.4**. Each PDC is 141 bytes with five draw commands.
SVG and PDC renders matched for all three sizes.

The launcher requires a 25×25 bitmap, so the native-size 25px PDC was rendered
to `resources/images/menu_icon.png`, preserving a transparent exterior and
opaque grayscale artwork. The existing `IMAGE_MENU_ICON` / `menuIcon: true`
resource registration remains in place. Conversion details are in
[`resources/launcher-icons/README.md`](../resources/launcher-icons/README.md).

Validation: production 0.7.3 compiled successfully with Pebble Tool 5.0.40 / SDK
4.33.1 for Emery, with the usual SDK ELF LOAD-segment RWX linker warning. RAM
footprint stays at 35,679 bytes; resources are 5,288 bytes. The application was
installed in the actual local Emery emulator, its icon inspected in the app
launcher, and the app reopened using the physical-button controls.

Evidence: [launcher](launcher-icon/launcher.png) and
[selected launcher row](launcher-icon/launcher-selected.png).
Physical-watch and phone-app icon-cache behavior remain to be checked after
installation. No live provider request or appstore release was made.

Pull `main` in CloudPebble, confirm **0.7.3**, build and run on **Phone** to install.
