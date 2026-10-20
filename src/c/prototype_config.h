#pragma once

// Approved UI is the default in both modes. CloudPebble may edit these values.
#ifndef PEBBLE_MOCK_DATA
#define PEBBLE_MOCK_DATA 0
#endif
#ifndef PEBBLE_USE_LEGACY_UI
#define PEBBLE_USE_LEGACY_UI 0
#endif
#define PEBBLE_UI_PROTOTYPE (!PEBBLE_USE_LEGACY_UI)

// Enabled only in the separately packaged offline motion review app.
#ifndef PEBBLE_MOTION_DEMO
#define PEBBLE_MOTION_DEMO 0
#endif
