#pragma once
#include <pebble.h>

// Returns false for an unsupported carrier, leaving the neutral header intact.
bool airline_icon_draw(GContext *ctx, const char *flight_number, GPoint origin);
// Caller owns the mutable copy; the cache remains immutable.
GDrawCommandImage *airline_icon_clone(const char *flight_number);
void airline_icons_deinit(void);
