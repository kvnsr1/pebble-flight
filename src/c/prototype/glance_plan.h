#pragma once
#include <stdbool.h>
#include <stdint.h>

#define GLANCE_MAX_SLICES 11
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
// Respect the capacity supplied by the OS without leaving a stale/false
// terminal subtitle when the full five-flight schedule does not fit.
bool glance_plan_pick(const GlancePlan *plan, int32_t now, unsigned limit,
                      unsigned position, GlanceEntry *entry);
