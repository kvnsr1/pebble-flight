#pragma once
#include <stdint.h>

enum {
  MOTION_UNIT = 1000,
  MOTION_SELECTION_MS = 110,
  MOTION_PAGE_MS = 190,
  MOTION_FLIGHT_MS = 320,
  MOTION_NOTE_MS = 150,
  MOTION_PLANE_MS = 140,
  MOTION_PROGRESS_MS = 220,
  MOTION_EMPHASIS_MS = 650
};

int motion_mix(int from, int to, int progress);
int motion_ease(int progress);
int motion_travel(int distance, int overshoot, int progress);

typedef struct { int logo_x, logo_y, number_x, number_y; } FlightHeaderPosition;
// Morph the horizontal card eyebrow into the vertical Overview stack. Move the
// text below the icon before bringing it under the icon, so they never overlap.
FlightHeaderPosition motion_flight_header(int row_y, int number_width, int progress);
