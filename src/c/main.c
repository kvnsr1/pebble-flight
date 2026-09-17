#include <pebble.h>

typedef struct {
  char flight[16];
  char date[24];
  char origin[8];
  char destination[8];
  char departure_time[16];
  char arrival_time[16];
  char status[32];
  char departure_gate[12];
  char departure_terminal[12];
  char arrival_gate[12];
  char arrival_terminal[12];
  char updated[16];
  char error[84];
  int status_level;
  bool loading;
} FlightData;

static Window *s_window;
static Layer *s_canvas;
static AppTimer *s_refresh_timer;
static AppTimer *s_loading_timer;
static FlightData s_flight;
static int s_page;
static int s_loading_frame;

#define REFRESH_INTERVAL_MS (10 * 60 * 1000)

static GColor status_color(void) {
  switch (s_flight.status_level) {
    case 0: return GColorIslamicGreen;
    case 1: return GColorChromeYellow;
    case 2: return GColorOrange;
    case 3: return GColorRed;
    default: return GColorDarkCandyAppleRed;
  }
}

static GColor status_text_color(void) {
  return s_flight.status_level >= 3 ? GColorWhite : GColorBlack;
}

static void copy_tuple(DictionaryIterator *iter, uint32_t key, char *dest, size_t size) {
  Tuple *tuple = dict_find(iter, key);
  if (tuple && tuple->type == TUPLE_CSTRING) {
    snprintf(dest, size, "%s", tuple->value->cstring);
  }
}

static void draw_text(GContext *ctx, const char *text, GRect rect, GFont font,
                      GColor color, GTextAlignment alignment) {
  graphics_context_set_text_color(ctx, color);
  graphics_draw_text(ctx, text, font, rect, GTextOverflowModeTrailingEllipsis,
                     alignment, NULL);
}

static void draw_page_dots(GContext *ctx, GRect bounds) {
  int start_x = bounds.size.w - 40;
  for (int i = 0; i < 3; i++) {
    graphics_context_set_fill_color(ctx, i == s_page ? GColorCyan : GColorDarkGray);
    graphics_fill_circle(ctx, GPoint(start_x + i * 11, 18), i == s_page ? 3 : 2);
  }
}

static void draw_header(GContext *ctx, GRect bounds, const char *title, GColor accent) {
  graphics_context_set_fill_color(ctx, accent);
  graphics_fill_rect(ctx, GRect(0, 0, 4, 36), 0, GCornerNone);
  draw_text(ctx, title, GRect(12, 5, bounds.size.w - 62, 28),
            fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GColorWhite,
            GTextAlignmentLeft);
  draw_page_dots(ctx, bounds);
}

static void draw_route_line(GContext *ctx, GRect bounds) {
  int y = 86;
  int left = 53;
  int right = bounds.size.w - 53;
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_line(ctx, GPoint(left, y), GPoint(right, y));
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, GPoint(left, y), 4);
  graphics_fill_circle(ctx, GPoint(right, y), 4);
  graphics_context_set_fill_color(ctx, GColorCyan);
  int travel_x = left + ((right - left) * s_loading_frame) / 11;
  graphics_fill_circle(ctx, GPoint(travel_x, y), s_flight.loading ? 4 : 3);
}

static void draw_sparkles(GContext *ctx, GRect bounds) {
  if (s_flight.status_level != 0 || s_flight.loading) { return; }
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(15, 121, 2, 6), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(13, 123, 6, 2), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(bounds.size.w - 17, 137, 2, 6), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(bounds.size.w - 19, 139, 6, 2), 0, GCornerNone);
}

static void canvas_update(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorOxfordBlue);
  graphics_fill_rect(ctx, b, 0, GCornerNone);

  if (s_page == 0) {
    draw_header(ctx, b, s_flight.flight, GColorCyan);
    draw_text(ctx, s_flight.date, GRect(10, 37, b.size.w - 20, 22),
              fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GColorLightGray,
              GTextAlignmentCenter);
    draw_text(ctx, s_flight.origin, GRect(8, 66, 64, 34),
              fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GColorWhite,
              GTextAlignmentLeft);
    draw_text(ctx, s_flight.destination, GRect(b.size.w - 72, 66, 64, 34),
              fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GColorWhite,
              GTextAlignmentRight);
    draw_route_line(ctx, b);
    graphics_context_set_fill_color(ctx, status_color());
    graphics_fill_rect(ctx, GRect(10, 108, b.size.w - 20, 42), 8, GCornersAll);
    draw_sparkles(ctx, b);
    draw_text(ctx, s_flight.status, GRect(23, 115, b.size.w - 46, 28),
              fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), status_text_color(),
              GTextAlignmentCenter);
    draw_text(ctx, "DEPART", GRect(10, 158, 82, 18),
              fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GColorLightGray,
              GTextAlignmentLeft);
    draw_text(ctx, "ARRIVE", GRect(b.size.w - 92, 158, 82, 18),
              fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GColorLightGray,
              GTextAlignmentRight);
    draw_text(ctx, s_flight.departure_time, GRect(10, 175, 90, 26),
              fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GColorWhite,
              GTextAlignmentLeft);
    draw_text(ctx, s_flight.arrival_time, GRect(b.size.w - 100, 175, 90, 26),
              fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GColorWhite,
              GTextAlignmentRight);
    char footer[32];
    snprintf(footer, sizeof(footer), s_flight.loading ? "LIVE  Updating route..." : "LIVE  Updated %s", s_flight.updated);
    draw_text(ctx, s_flight.error[0] ? s_flight.error : footer,
              GRect(8, 205, b.size.w - 16, 20), fonts_get_system_font(FONT_KEY_GOTHIC_14),
              s_flight.error[0] ? GColorRed : GColorLightGray, GTextAlignmentCenter);
  } else {
    bool departure = s_page == 1;
    GColor accent = departure ? GColorCyan : GColorChromeYellow;
    draw_header(ctx, b, departure ? "DEPARTURE" : "ARRIVAL", accent);
    const char *airport = departure ? s_flight.origin : s_flight.destination;
    const char *time = departure ? s_flight.departure_time : s_flight.arrival_time;
    const char *terminal = departure ? s_flight.departure_terminal : s_flight.arrival_terminal;
    const char *gate = departure ? s_flight.departure_gate : s_flight.arrival_gate;
    draw_text(ctx, airport, GRect(10, 44, b.size.w - 20, 42),
              fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK), GColorWhite,
              GTextAlignmentCenter);
    draw_text(ctx, time, GRect(10, 82, b.size.w - 20, 34),
              fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), accent,
              GTextAlignmentCenter);
    graphics_context_set_fill_color(ctx, GColorDarkGray);
    graphics_fill_rect(ctx, GRect(10, 126, 84, 82), 8, GCornersAll);
    graphics_fill_rect(ctx, GRect(106, 126, 84, 82), 8, GCornersAll);
    draw_text(ctx, "TERMINAL", GRect(14, 135, 76, 22),
              fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GColorLightGray,
              GTextAlignmentCenter);
    draw_text(ctx, "GATE", GRect(110, 135, 76, 22),
              fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GColorLightGray,
              GTextAlignmentCenter);
    draw_text(ctx, terminal, GRect(14, 160, 76, 40),
              fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD), GColorWhite,
              GTextAlignmentCenter);
    draw_text(ctx, gate, GRect(110, 160, 76, 40),
              fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD), GColorWhite,
              GTextAlignmentCenter);
    draw_text(ctx, departure ? "Ready when you are" : "Welcome home", GRect(10, 209, b.size.w - 20, 18),
              fonts_get_system_font(FONT_KEY_GOTHIC_14), GColorLightGray,
              GTextAlignmentCenter);
  }
}

static void loading_timer_callback(void *context) {
  if (!s_flight.loading) {
    s_loading_timer = NULL;
    return;
  }
  s_loading_frame = (s_loading_frame + 1) % 12;
  layer_mark_dirty(s_canvas);
  s_loading_timer = app_timer_register(140, loading_timer_callback, NULL);
}

static void start_loading_animation(void) {
  s_loading_frame = 0;
  if (!s_loading_timer) {
    s_loading_timer = app_timer_register(140, loading_timer_callback, NULL);
  }
}

static void change_page(int delta) {
  s_page = (s_page + delta + 3) % 3;
  layer_mark_dirty(s_canvas);
}

static void up_click(ClickRecognizerRef recognizer, void *context) { change_page(-1); }
static void down_click(ClickRecognizerRef recognizer, void *context) { change_page(1); }

static void request_refresh(void) {
  DictionaryIterator *out;
  if (app_message_outbox_begin(&out) == APP_MSG_OK) {
    dict_write_uint8(out, MESSAGE_KEY_REQUEST_REFRESH, 1);
    app_message_outbox_send();
    s_flight.loading = true;
    s_flight.error[0] = '\0';
    start_loading_animation();
    layer_mark_dirty(s_canvas);
  }
}

static void refresh_timer_callback(void *context) {
  request_refresh();
  s_refresh_timer = app_timer_register(REFRESH_INTERVAL_MS, refresh_timer_callback, NULL);
}

static void select_click(ClickRecognizerRef recognizer, void *context) { request_refresh(); }

static void select_long_click(ClickRecognizerRef recognizer, void *context) {
  DictionaryIterator *out;
  if (app_message_outbox_begin(&out) == APP_MSG_OK) {
    dict_write_uint8(out, MESSAGE_KEY_REQUEST_NEXT_FLIGHT, 1);
    app_message_outbox_send();
    s_flight.loading = true;
    s_flight.error[0] = '\0';
    start_loading_animation();
    vibes_short_pulse();
    layer_mark_dirty(s_canvas);
  }
}

static void click_config(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click);
  window_long_click_subscribe(BUTTON_ID_SELECT, 700, select_long_click, NULL);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click);
}

static void inbox_received(DictionaryIterator *iter, void *context) {
  copy_tuple(iter, MESSAGE_KEY_FLIGHT_NUMBER, s_flight.flight, sizeof(s_flight.flight));
  copy_tuple(iter, MESSAGE_KEY_FLIGHT_DATE, s_flight.date, sizeof(s_flight.date));
  copy_tuple(iter, MESSAGE_KEY_ORIGIN, s_flight.origin, sizeof(s_flight.origin));
  copy_tuple(iter, MESSAGE_KEY_DESTINATION, s_flight.destination, sizeof(s_flight.destination));
  copy_tuple(iter, MESSAGE_KEY_DEPARTURE_TIME, s_flight.departure_time, sizeof(s_flight.departure_time));
  copy_tuple(iter, MESSAGE_KEY_ARRIVAL_TIME, s_flight.arrival_time, sizeof(s_flight.arrival_time));
  copy_tuple(iter, MESSAGE_KEY_STATUS_LABEL, s_flight.status, sizeof(s_flight.status));
  copy_tuple(iter, MESSAGE_KEY_DEPARTURE_GATE, s_flight.departure_gate, sizeof(s_flight.departure_gate));
  copy_tuple(iter, MESSAGE_KEY_DEPARTURE_TERMINAL, s_flight.departure_terminal, sizeof(s_flight.departure_terminal));
  copy_tuple(iter, MESSAGE_KEY_ARRIVAL_GATE, s_flight.arrival_gate, sizeof(s_flight.arrival_gate));
  copy_tuple(iter, MESSAGE_KEY_ARRIVAL_TERMINAL, s_flight.arrival_terminal, sizeof(s_flight.arrival_terminal));
  copy_tuple(iter, MESSAGE_KEY_UPDATED_AT, s_flight.updated, sizeof(s_flight.updated));
  copy_tuple(iter, MESSAGE_KEY_ERROR_MESSAGE, s_flight.error, sizeof(s_flight.error));
  Tuple *level = dict_find(iter, MESSAGE_KEY_STATUS_LEVEL);
  Tuple *loading = dict_find(iter, MESSAGE_KEY_IS_LOADING);
  if (level) { s_flight.status_level = (int)level->value->int32; }
  if (loading) { s_flight.loading = loading->value->int32 != 0; }
  if (s_flight.loading) { start_loading_animation(); }
  layer_mark_dirty(s_canvas);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_canvas = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_canvas, canvas_update);
  layer_add_child(root, s_canvas);
}

static void window_unload(Window *window) { layer_destroy(s_canvas); }

static void init(void) {
  snprintf(s_flight.flight, sizeof(s_flight.flight), "PEBBLE FLIGHT");
  snprintf(s_flight.date, sizeof(s_flight.date), "Configure on phone");
  snprintf(s_flight.origin, sizeof(s_flight.origin), "---");
  snprintf(s_flight.destination, sizeof(s_flight.destination), "---");
  snprintf(s_flight.departure_time, sizeof(s_flight.departure_time), "--");
  snprintf(s_flight.arrival_time, sizeof(s_flight.arrival_time), "--");
  snprintf(s_flight.status, sizeof(s_flight.status), "SET UP FLIGHT");
  snprintf(s_flight.departure_gate, sizeof(s_flight.departure_gate), "--");
  snprintf(s_flight.departure_terminal, sizeof(s_flight.departure_terminal), "--");
  snprintf(s_flight.arrival_gate, sizeof(s_flight.arrival_gate), "--");
  snprintf(s_flight.arrival_terminal, sizeof(s_flight.arrival_terminal), "--");
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){.load = window_load, .unload = window_unload});
  window_set_click_config_provider(s_window, click_config);
  window_stack_push(s_window, true);
  app_message_register_inbox_received(inbox_received);
  app_message_open(1024, 128);
  s_refresh_timer = app_timer_register(REFRESH_INTERVAL_MS, refresh_timer_callback, NULL);
}

static void deinit(void) {
  if (s_refresh_timer) { app_timer_cancel(s_refresh_timer); }
  if (s_loading_timer) { app_timer_cancel(s_loading_timer); }
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
