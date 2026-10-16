#pragma once
#include <pebble.h>
void pebble_plane_init(void);
void pebble_plane_draw(GContext *ctx, GPoint center);
void pebble_plane_deinit(void);
