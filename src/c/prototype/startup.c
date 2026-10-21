#include "../prototype_config.h"
#if PEBBLE_UI_PROTOTYPE
#include "startup.h"
#include "pebble_plane.h"
#include "motion.h"

enum { DURATION_MS = 3300, FLY_MS = 1300, TITLE_MS = 1780, REST_MS = 2600 };
static Layer *s_layer;
static Animation *s_animation;
static GDrawCommandImage *s_plane;
static int s_elapsed;
static bool s_active;

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

static void draw(Layer *layer, GContext *ctx) {
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

static void stopped(Animation *animation, bool finished, void *context) {
  (void)animation; (void)finished; (void)context;
  s_animation = NULL;
  s_active = false;
  if (s_layer) { layer_set_hidden(s_layer, true); }
  if (s_plane) { gdraw_command_image_destroy(s_plane); s_plane = NULL; }
}

static void update(Animation *animation, AnimationProgress value) {
  (void)animation;
  int elapsed = (int)((int64_t)value * DURATION_MS / ANIMATION_NORMALIZED_MAX);
  // Once settled, keep the title still through its final reading pause.
  bool resting = s_elapsed >= REST_MS && elapsed >= REST_MS;
  s_elapsed = elapsed;
  if (!resting && s_layer) { layer_mark_dirty(s_layer); }
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
}

void startup_show(Layer *root) {
  startup_destroy();
  s_layer = layer_create(layer_get_bounds(root));
  if (!s_layer) { return; }
  s_animation = animation_create();
  if (!s_animation) { startup_destroy(); return; }
  s_elapsed = 0;
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
