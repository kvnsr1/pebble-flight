#pragma once
#include <stdbool.h>
#include <stdint.h>

#define GLANCE_MAX_SLICES 7
#define GLANCE_TEXT_BYTES 150
typedef struct {
  int32_t expires;
  char text[GLANCE_TEXT_BYTES+1];
} GlanceEntry;
typedef struct {
  unsigned count;
  GlanceEntry entries[GLANCE_MAX_SLICES];
} GlancePlan;

// Atomic wire format: one UTC-seconds<TAB>subtitle<NEWLINE> per slice.
bool glance_plan_parse(const char *wire, GlancePlan *plan);
