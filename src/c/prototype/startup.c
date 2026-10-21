#include "../prototype_config.h"
#if PEBBLE_UI_PROTOTYPE
#include "startup.h"
#include "pebble_plane.h"
#include "motion.h"

enum { DURATION_MS = 4000, FLY_MS = 1300, TITLE_MS = 1780,
       EXIT_MS = 2850, MAX_WIDTH = 200 };
static Layer *s_layer, *s_root;
static Animation *s_animation;
static GDrawCommandImage *s_plane;
static GBitmap *s_transition;
static int s_elapsed;
static bool s_active, s_exit_failed;

static int progress(int elapsed, int start, int duration) {
  int p = (elapsed - start) * MOTION_UNIT / duration;
  return p < 0 ? 0 : p > MOTION_UNIT ? MOTION_UNIT : p;
}

// A cloud is a small group of overlapping native circles. Its lobes drift at
// different speeds as it dissipates, instead of wiping the frame or blinking.
static void cloud(GContext *ctx, int x, int y, int size, int age, int clear, bool upper) {
  int grow = motion_ease(progress(age, 0, 240));
  int remain = MOTION_UNIT - clear;
  int radius = size * grow * remain / 1000000;
  if (radius < 1) { return; }
  int drift = clear * clear / MOTION_UNIT;
  x += motion_mix(0, upper ? -20 : 28, drift);
  y += motion_mix(0, upper ? -49 : 51, drift);
  // Under-cloud shade gives the white puffs a little volume on the green sky.
  graphics_context_set_fill_color(ctx, GColorMintGreen);
  graphics_fill_circle(ctx, GPoint(x, y + radius / 3), radius);
  graphics_fill_circle(ctx, GPoint(x + radius, y + radius / 3), radius * 3 / 4);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, GPoint(x - radius * 3 / 4, y), radius * 3 / 4);
  graphics_fill_circle(ctx, GPoint(x, y - radius / 3), radius);
  graphics_fill_circle(ctx, GPoint(x + radius * 4 / 5, y), radius * 4 / 5);
}

static void draw_intro(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int w = bounds.size.w, h = bounds.size.h;
  int center = h / 2;
  graphics_context_set_fill_color(ctx, GColorDarkGreen);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Native Gothic is the same Pebble font used for the app's page headings.
  // Draw behind the cloud bank: the parting clouds reveal the settled title.
  if (s_elapsed >= TITLE_MS) {
    int p = motion_ease(progress(s_elapsed, TITLE_MS, 620));
    graphics_context_set_text_color(ctx, p < 350 ? GColorMintGreen : GColorWhite);
    graphics_draw_text(ctx, "Jetteble", fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD),
                       GRect(0, center - 22 + motion_mix(5, 0, p), w, 42),
                       GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  }

  // Emit clouds only after the tail passes their origin. Alternating sizes,
  // release times and upward/downward drift keep the clearing asymmetric.
  const int sizes[] = {13, 17, 15, 19, 16, 18, 14, 16};
  const int offsets[] = {3, -6, 7, -5, 5, -7, 4, -3};
  for (int i = 0; i < 8; ++i) {
    int x = 7 + i * (w - 14) / 7;
    int born = (x + 82) * FLY_MS / (w + 100);
    int age = s_elapsed - born;
    if (age < 0) { continue; }
    int clear = motion_ease(progress(s_elapsed, 1450 + i * 48, 820));
    cloud(ctx, x, center + offsets[i], sizes[i], age, clear, i % 2 == 0);
  }

  if (s_elapsed < FLY_MS) {
    int p = progress(s_elapsed, 0, FLY_MS);
    int x = motion_mix(-50, w + 50, p);
    // A gentle four-pixel climb and settle, while maintaining rightward travel.
    int lift = p < 500 ? motion_mix(0, 4, p * 2) : motion_mix(4, 0, (p - 500) * 2);
    if (s_plane) {
      gdraw_command_image_draw(ctx, s_plane, GPoint(x - 25, center - 25 - lift));
    } else {
      pebble_plane_draw(ctx, GPoint(x, center - lift));
    }
  }
}

static int smooth(int p) {
  // Smooth acceleration followed by a soft landing; all positions stay integral.
  return (int)((int64_t)p * p * (3000 - 2 * p) / 1000000);
}

static void set_pixel(uint8_t *row, int x, uint8_t value) {
  int shift = (1 - (x & 1)) * 4;
  row[x / 2] = (row[x / 2] & ~(15 << shift)) | (value << shift);
}

static bool prepare_transition(GContext *ctx, GRect bounds) {
  if (bounds.size.w > MAX_WIDTH) { return false; }
  s_transition = gbitmap_create_blank(bounds.size, GBitmapFormat4BitPalette);
  if (!s_transition) { return false; }
  // Three source colors, each at the panel's four native alpha levels. A 4-bit
  // surface costs about 23 KB on Emery and is released as soon as the intro ends.
  GColor *palette = gbitmap_get_palette(s_transition);
  const GColor colors[] = {GColorDarkGreen, GColorWhite, GColorMintGreen, GColorWhite};
  for (int i = 0; i < 16; ++i) {
    palette[i] = colors[i % 4];
    palette[i].a = i / 4;
  }
  // Capture only our settled green/title composition once. The actual app is
  // redrawn underneath every following frame, so incoming data stays current.
  GBitmap *frame = graphics_capture_frame_buffer_format(ctx, GBitmapFormat8Bit);
  if (!frame) { gbitmap_destroy(s_transition); s_transition = NULL; return false; }
  uint8_t *data = gbitmap_get_data(s_transition);
  int stride = gbitmap_get_bytes_per_row(s_transition);
  for (int y = 0; y < bounds.size.h; ++y) {
    GBitmapDataRowInfo source = gbitmap_get_data_row_info(frame, y);
    for (int x = 0; x < bounds.size.w; ++x) {
      int color = (source.data[x] & 63) == (GColorWhite.argb & 63) ? 1 : 0;
      set_pixel(data + y * stride, x, 12 + color);
    }
  }
  graphics_release_frame_buffer(ctx, frame);
  return true;
}

static void add_lobe(int16_t *edge, int w, int cx, int cy, int radius) {
  int dy = radius;
  for (int dx = 0; dx <= radius; ++dx) {
    while (dx * dx + dy * dy > radius * radius) { --dy; }
    int left = cx - dx, right = cx + dx;
    if (left >= 0 && left < w && cy - dy < edge[left]) { edge[left] = cy - dy; }
    if (right >= 0 && right < w && cy - dy < edge[right]) { edge[right] = cy - dy; }
  }
}

static void draw_transition(GContext *ctx, GRect bounds) {
  int w = bounds.size.w, h = bounds.size.h;
  int16_t edge[MAX_WIDTH];
  for (int x = 0; x < w; ++x) { edge[x] = h + 100; }
  // A center-led cascade, with different release times, sizes and sideways
  // drift. The front grows into rounded lobes instead of a straight screen wipe.
  const int delays[] = {95, 35, 115, 0, 75, 45, 125};
  const int sizes[] = {38, 44, 40, 48, 42, 46, 40};
  for (int i = 0; i < 7; ++i) {
    int p = progress(s_elapsed, EXIT_MS + delays[i], 780);
    int rise = smooth(p);
    int x = -12 + i * (w + 24) / 6 + motion_mix(0, i % 2 ? 7 : -7, rise);
    int y = motion_mix(h + 62, -180, rise);
    int radius = motion_mix(24, sizes[i], motion_ease(p));
    add_lobe(edge, w, x, y, radius);
    add_lobe(edge, w, x - radius / 2, y + 15, radius * 3 / 4);
    add_lobe(edge, w, x + radius / 2, y + 12, radius * 4 / 5);
  }

  uint8_t *data = gbitmap_get_data(s_transition);
  int stride = gbitmap_get_bytes_per_row(s_transition);
  int dissolve = MOTION_UNIT - smooth(progress(s_elapsed, EXIT_MS + 760, 300));
  for (int y = 0; y < h; ++y) {
    uint8_t *row = data + y * stride;
    for (int x = 0; x < w; ++x) {
      if (y < edge[x]) { continue; }
      int depth = y - edge[x];
      int opacity = (MOTION_UNIT - smooth(progress(depth, 44, 112))) * dissolve / MOTION_UNIT;
      // Use the panel's real alpha levels across a moving soft trailing edge.
      // Keep the cloud surface clean instead of introducing a dither pattern.
      int alpha = (opacity * 3 + MOTION_UNIT / 2) / MOTION_UNIT;
      set_pixel(row, x, alpha * 4 + (depth < 44 ? 1 : 2));
    }
  }
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, s_transition, bounds);
  graphics_context_set_compositing_mode(ctx, GCompOpAssign);
}

static void draw(Layer *layer, GContext *ctx) {
  if (s_elapsed < EXIT_MS) { draw_intro(layer, ctx); return; }
  if (!s_transition && !s_exit_failed) {
    draw_intro(layer, ctx);
    s_exit_failed = !prepare_transition(ctx, layer_get_bounds(layer));
  }
  if (s_transition) { draw_transition(ctx, layer_get_bounds(layer)); }
}

static void stopped(Animation *animation, bool finished, void *context) {
  (void)animation; (void)finished; (void)context;
  s_animation = NULL;
  s_active = false;
  if (s_layer) { layer_set_hidden(s_layer, true); }
  if (s_plane) { gdraw_command_image_destroy(s_plane); s_plane = NULL; }
  if (s_transition) { gbitmap_destroy(s_transition); s_transition = NULL; }
}

static void update(Animation *animation, AnimationProgress value) {
  (void)animation;
  int elapsed = (int)((int64_t)value * DURATION_MS / ANIMATION_NORMALIZED_MAX);
  s_elapsed = elapsed;
  if (s_exit_failed) { animation_unschedule(s_animation); return; }
  // Transparency requires a fresh app frame underneath, including live updates.
  if (s_elapsed >= EXIT_MS && s_root) { layer_mark_dirty(s_root); }
  else if (s_layer) { layer_mark_dirty(s_layer); }
}
static const AnimationImplementation s_implementation = { .update = update };

bool startup_dismiss(void) {
  if (!s_active) { return false; }
  if (s_animation) { animation_unschedule(s_animation); }
  return true;
}

void startup_destroy(void) {
  startup_dismiss();
  if (s_layer) { layer_destroy(s_layer); s_layer = NULL; }
  s_root = NULL;
}

void startup_show(Layer *root) {
  startup_destroy();
  s_layer = layer_create(layer_get_bounds(root));
  if (!s_layer) { return; }
  s_animation = animation_create();
  if (!s_animation) { startup_destroy(); return; }
  s_elapsed = 0;
  s_root = root;
  s_exit_failed = false;
  s_active = true;
  s_plane = gdraw_command_image_create_with_resource(RESOURCE_ID_PROGRESS_PLANE_50);
  layer_set_update_proc(s_layer, draw);
  layer_add_child(root, s_layer);
  animation_set_duration(s_animation, DURATION_MS);
  animation_set_curve(s_animation, AnimationCurveLinear);
  animation_set_implementation(s_animation, &s_implementation);
  animation_set_handlers(s_animation, (AnimationHandlers){ .stopped = stopped }, NULL);
  animation_schedule(s_animation);
}
#endif
