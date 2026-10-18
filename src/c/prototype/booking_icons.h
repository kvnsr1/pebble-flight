#pragma once
#include <pebble.h>
#include "flight_model.h"

void booking_icons_init(void);
void booking_icons_deinit(void);
void booking_code_icon_draw(GContext *ctx, GPoint origin);
void booking_seat_icon_draw(GContext *ctx, GPoint origin, SeatPosition position);
