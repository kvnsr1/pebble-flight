#include "motion.h"

FlightHeaderPosition motion_flight_header(int row_y, int number_width, int progress) {
  int p=motion_ease(progress);
  int vertical=motion_ease(progress*2);
  int horizontal=motion_ease((progress-500)*2);
  FlightHeaderPosition position;
  position.logo_x=motion_mix(12,86,p);
  position.logo_y=motion_mix(row_y+3,66,p);
  position.number_x=position.logo_x+motion_mix(32,12-number_width/2,horizontal);
  position.number_y=position.logo_y+motion_mix(-1,25,vertical);
  return position;
}

static int clamp(int n) { return n < 0 ? 0 : n > MOTION_UNIT ? MOTION_UNIT : n; }

int motion_mix(int from, int to, int progress) {
  return from + (int)((int64_t)(to - from) * clamp(progress) / MOTION_UNIT);
}

int motion_ease(int progress) {
  int remaining = MOTION_UNIT - clamp(progress);
  return MOTION_UNIT - (int)((int64_t)remaining * remaining * remaining / 1000000);
}

// One small overshoot and one settle; integers keep the original art pixel aligned.
int motion_travel(int distance, int overshoot, int progress) {
  int p = clamp(progress);
  if (p < 780) {
    return motion_mix(0, distance + overshoot, motion_ease(p * MOTION_UNIT / 780));
  }
  return motion_mix(distance + overshoot, distance, (p - 780) * MOTION_UNIT / 220);
}
