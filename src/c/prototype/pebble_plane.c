#include "../prototype_config.h"
#if PEBBLE_UI_PROTOTYPE
#include "pebble_plane.h"

// Native GPath translation of Pebble_25x25_Scheduled_flight.svg, PebbleOS.
// Apache-2.0. Original SVG and license are in resources/pebble-iconography/.
// Modification: SVG coordinates translated to integer GPaths (14.5 -> 15).
static GPoint s_body_points[] = {{3,17},{20,17},{24,13},{24,8},{20,4},{6,10},{4,7},{1,7}};
static GPoint s_wing_points[] = {{9,15},{9,21},{14,21},{17,12},{14,12}};
static GPath *s_body, *s_wing;
void pebble_plane_init(void) {
  s_body = gpath_create(&(GPathInfo){8, s_body_points});
  s_wing = gpath_create(&(GPathInfo){5, s_wing_points});
}
static void draw(GContext *ctx, GPoint center, GColor fill, GColor stroke) {
  if (!s_body || !s_wing) { return; }
  GPoint offset = GPoint(center.x - 12, center.y - 12);
  gpath_move_to(s_body, offset);
  gpath_move_to(s_wing, offset);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_context_set_stroke_color(ctx, stroke);
  graphics_context_set_fill_color(ctx, fill);
  gpath_draw_filled(ctx, s_body);
  gpath_draw_outline(ctx, s_body);
  gpath_draw_filled(ctx, s_wing);
  gpath_draw_outline_open(ctx, s_wing);
  graphics_draw_line(ctx, GPoint(offset.x + 22, offset.y + 6),
                     GPoint(offset.x + 20, offset.y + 8));
}
void pebble_plane_draw(GContext *ctx, GPoint center) {
  draw(ctx,center,GColorWhite,GColorBlack);
}
void pebble_plane_badge_draw(GContext *ctx, GPoint center) {
  graphics_context_set_fill_color(ctx,GColorWhite);
  graphics_fill_circle(ctx,center,13);
  draw(ctx,center,GColorDarkGreen,GColorDarkGreen);
}
void pebble_plane_deinit(void) {
  if (s_body) { gpath_destroy(s_body); }
  if (s_wing) { gpath_destroy(s_wing); }
}
#endif
