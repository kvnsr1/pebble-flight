#pragma once
#include <stdint.h>
#include "../prototype_config.h"

enum {
  MOTION_UNIT = 1000,
  MOTION_BANDS = 4,
  MOTION_BAND_STAGGER = 55, // Master-timeline units; 23ms per band on opening.
  MOTION_BAND_LIMIT = 3,    // Maximum internal vector lag in pixels.
  MOTION_HERO_ICON_GROW = 10,
  MOTION_HERO_TEXT_GROW = 120, // 12% scale; text is never internally deformed.
  MOTION_FOCUS_ICON_GROW = 4,
  MOTION_FOCUS_TEXT_GROW = 80,
  MOTION_FOCUS_DELAY = 100, // 28ms on the 280ms focus timeline.
  MOTION_SELECTION_MS = PEBBLE_STRETCH_MOTION ? 280 : 110,
  MOTION_PAGE_MS = 190,
  MOTION_FLIGHT_MS = PEBBLE_STRETCH_MOTION ? 420 : 320,
  MOTION_NOTE_MS = 150,
  MOTION_PLANE_MS = 140,
  MOTION_PROGRESS_MS = 220,
  MOTION_EMPHASIS_MS = 650
};

int motion_mix(int from, int to, int progress);
int motion_ease(int progress);
int motion_smooth(int progress);
int motion_phase(int progress, int start, int end);
int motion_pulse(int progress);
int motion_band(int coordinate, int extent, int reverse);
int motion_band_offset(int progress, int band);
int motion_travel(int distance, int overshoot, int progress);

typedef struct { int logo_x, logo_y, number_x, number_y, logo_size, number_scale; } FlightHeaderPosition;
// Morph the horizontal card eyebrow into the vertical Overview stack. Move the
// text below the icon before bringing it under the icon, so they never overlap.
FlightHeaderPosition motion_flight_header(int row_y, int number_width, int progress);
