#include "motion.h"

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
