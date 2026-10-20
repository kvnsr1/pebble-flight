#include "../prototype_config.h"
#if PEBBLE_UI_PROTOTYPE
#include "pebble_plane.h"

// Native GPath translation of Pebble_25x25_Scheduled_flight.svg, PebbleOS.
// Apache-2.0. Original SVG and license are in resources/pebble-iconography/.
// Modification: SVG coordinates translated to integer GPaths (14.5 -> 15).
static GPoint s_body_points[] = {{3,17},{20,17},{24,13},{24,8},{20,4},{6,10},{4,7},{1,7}};
static GPoint s_wing_points[] = {{9,15},{9,21},{14,21},{17,12},{14,12}};
static GPath *s_body, *s_wing;
static GDrawCommandImage *s_scheduled, *s_in_flight;
enum { MARKER_SIZE = 25 };

static int16_t scale_coordinate(int16_t value, int16_t source_size) {
  int32_t scaled = value * MARKER_SIZE;
  return (scaled + (scaled < 0 ? -source_size / 2 : source_size / 2)) / source_size;
}

static bool scale_command(GDrawCommand *command, uint32_t index, void *context) {
  (void)index;
  int16_t source_size = *(int16_t *)context;
  // Both integer paths and PrecisePath's 13.3 coordinates scale in their own units.
  for (uint16_t i = 0; i < gdraw_command_get_num_points(command); ++i) {
    GPoint point = gdraw_command_get_point(command, i);
    gdraw_command_set_point(command, i, GPoint(scale_coordinate(point.x, source_size),
                                             scale_coordinate(point.y, source_size)));
  }
  uint8_t stroke = gdraw_command_get_stroke_width(command);
  if (stroke) {
    int16_t width = scale_coordinate(stroke, source_size);
    gdraw_command_set_stroke_width(command, width > 0 ? width : 1);
  }
  return true;
}

static GDrawCommandImage *load_marker(uint32_t resource) {
  GDrawCommandImage *image = gdraw_command_image_create_with_resource(resource);
  if (!image) { return NULL; }
  int16_t source_size = gdraw_command_image_get_bounds_size(image).w;
  // The original square 50/80 px resources stay intact; fit either into the
  // accepted marker footprint once at load time, rather than during every draw.
  gdraw_command_list_iterate(gdraw_command_image_get_command_list(image),
                             scale_command, &source_size);
  gdraw_command_image_set_bounds_size(image, GSize(MARKER_SIZE, MARKER_SIZE));
  return image;
}

void pebble_plane_init(void) {
  s_body = gpath_create(&(GPathInfo){8, s_body_points});
  s_wing = gpath_create(&(GPathInfo){5, s_wing_points});
  s_scheduled = load_marker(RESOURCE_ID_PROGRESS_PLANE_50);
  s_in_flight = load_marker(RESOURCE_ID_PROGRESS_PLANE_80);
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
void pebble_plane_progress_draw(GContext *ctx, GPoint center, bool in_flight) {
  GDrawCommandImage *image = in_flight ? s_in_flight : s_scheduled;
  if (image) {
    gdraw_command_image_draw(ctx, image, GPoint(center.x - MARKER_SIZE / 2,
                                             center.y - MARKER_SIZE / 2));
  } else {
    pebble_plane_draw(ctx, center);
  }
}
void pebble_plane_deinit(void) {
  if (s_body) { gpath_destroy(s_body); }
  if (s_wing) { gpath_destroy(s_wing); }
  if (s_scheduled) { gdraw_command_image_destroy(s_scheduled); }
  if (s_in_flight) { gdraw_command_image_destroy(s_in_flight); }
}
#endif
