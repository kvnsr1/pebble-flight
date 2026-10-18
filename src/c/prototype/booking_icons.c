#include "../prototype_config.h"
#if PEBBLE_UI_PROTOTYPE
#include "booking_icons.h"

static GDrawCommandImage *s_boarding_pass;
static GDrawCommandImage *s_seats;

void booking_icons_init(void) {
  s_boarding_pass=gdraw_command_image_create_with_resource(RESOURCE_ID_BOOKING_PASS_25);
  s_seats=gdraw_command_image_create_with_resource(RESOURCE_ID_BOOKING_SEATS_80);
}

void booking_icons_deinit(void) {
  if (s_boarding_pass) { gdraw_command_image_destroy(s_boarding_pass); s_boarding_pass=NULL; }
  if (s_seats) { gdraw_command_image_destroy(s_seats); s_seats=NULL; }
}

void booking_code_icon_draw(GContext *ctx, GPoint origin) {
  if (s_boarding_pass) { gdraw_command_image_draw(ctx,s_boarding_pass,origin); }
}

void booking_seat_icon_draw(GContext *ctx, GPoint origin, SeatPosition position) {
  if (!s_seats) { return; }
  GDrawCommandList *commands=gdraw_command_image_get_command_list(s_seats);
  // The unchanged 80px SVG has 28 paths. Backrests, headrests and cushions
  // are paths 5..7, 8..10 and 21..23 respectively (window, middle, aisle).
  // Reset each time so switching flights cannot leave a previous seat selected.
  const uint8_t seat_parts[]={5,8,21};
  const SeatPosition positions[]={SEAT_WINDOW,SEAT_MIDDLE,SEAT_AISLE};
  for (unsigned seat=0; seat<3; seat++) {
    bool selected=position==positions[seat];
    for (unsigned part=0; part<ARRAY_LENGTH(seat_parts); part++) {
      GDrawCommand *command=gdraw_command_list_get_command(commands,seat_parts[part]+seat);
      gdraw_command_set_fill_color(command,selected?GColorMalachite:GColorWhite);
      gdraw_command_set_stroke_color(command,selected?GColorWhite:GColorBlack);
    }
  }
  gdraw_command_image_draw(ctx,s_seats,origin);
  // Preserve the existing non-color selection cue. Unknown/N/A select no seat.
  int selected=position==SEAT_WINDOW?0:position==SEAT_MIDDLE?1:position==SEAT_AISLE?2:-1;
  if (selected>=0) {
    int x=origin.x+30+17*selected;
    graphics_context_set_stroke_color(ctx,GColorDarkGreen);
    graphics_context_set_stroke_width(ctx,2);
    graphics_draw_line(ctx,GPoint(x-4,origin.y+33),GPoint(x-1,origin.y+36));
    graphics_draw_line(ctx,GPoint(x-1,origin.y+36),GPoint(x+4,origin.y+30));
  }
}
#endif
