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
static FlightData s_flight;
static int s_page;

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

static void draw_header(GContext *ctx, GRect bounds, const char *title) {
  graphics_context_set_fill_color(ctx, GColorOxfordBlue);
  graphics_fill_rect(ctx, GRect(0, 0, bounds.size.w, 33), 0, GCornerNone);
  draw_text(ctx, title, GRect(10, 4, bounds.size.w - 20, 28),
            fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GColorWhite,
            GTextAlignmentLeft);
  char page[16];
  snprintf(page, sizeof(page), "%d/3", s_page + 1);
  draw_text(ctx, page, GRect(bounds.size.w - 50, 7, 40, 24),
            fonts_get_system_font(FONT_KEY_GOTHIC_18), GColorLightGray,
            GTextAlignmentRight);
}

static void canvas_update(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, b, 0, GCornerNone);

  if (s_page == 0) {
    draw_header(ctx, b, s_flight.flight);
    draw_text(ctx, s_flight.date, GRect(10, 38, b.size.w - 20, 24),
              fonts_get_system_font(FONT_KEY_GOTHIC_18), GColorDarkGray,
              GTextAlignmentCenter);
    char route[24];
    snprintf(route, sizeof(route), "%s  >  %s", s_flight.origin, s_flight.destination);
    draw_text(ctx, route, GRect(6, 62, b.size.w - 12, 48),
              fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD), GColorBlack,
              GTextAlignmentCenter);
    graphics_context_set_fill_color(ctx, status_color());
    graphics_fill_rect(ctx, GRect(10, 112, b.size.w - 20, 42), 6, GCornersAll);
    draw_text(ctx, s_flight.status, GRect(14, 119, b.size.w - 28, 30),
              fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GColorBlack,
              GTextAlignmentCenter);
    char times[40];
    snprintf(times, sizeof(times), "%s  -  %s", s_flight.departure_time, s_flight.arrival_time);
    draw_text(ctx, times, GRect(6, 162, b.size.w - 12, 28),
              fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GColorBlack,
              GTextAlignmentCenter);
    char footer[32];
    snprintf(footer, sizeof(footer), s_flight.loading ? "Refreshing..." : "Updated %s", s_flight.updated);
    draw_text(ctx, s_flight.error[0] ? s_flight.error : footer,
              GRect(8, 194, b.size.w - 16, 32), fonts_get_system_font(FONT_KEY_GOTHIC_14),
              s_flight.error[0] ? GColorRed : GColorDarkGray, GTextAlignmentCenter);
  } else {
    bool departure = s_page == 1;
    draw_header(ctx, b, departure ? "DEPARTURE" : "ARRIVAL");
    const char *airport = departure ? s_flight.origin : s_flight.destination;
    const char *time = departure ? s_flight.departure_time : s_flight.arrival_time;
    const char *terminal = departure ? s_flight.departure_terminal : s_flight.arrival_terminal;
    const char *gate = departure ? s_flight.departure_gate : s_flight.arrival_gate;
    draw_text(ctx, airport, GRect(10, 44, b.size.w - 20, 44),
              fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD), GColorBlack,
              GTextAlignmentCenter);
    draw_text(ctx, time, GRect(10, 84, b.size.w - 20, 38),
              fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GColorOxfordBlue,
              GTextAlignmentCenter);
    draw_text(ctx, "TERMINAL", GRect(14, 131, 80, 24),
              fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GColorDarkGray,
              GTextAlignmentCenter);
    draw_text(ctx, "GATE", GRect(106, 131, 80, 24),
              fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GColorDarkGray,
              GTextAlignmentCenter);
    draw_text(ctx, terminal, GRect(14, 154, 80, 46),
              fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD), GColorBlack,
              GTextAlignmentCenter);
    draw_text(ctx, gate, GRect(106, 154, 80, 46),
              fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD), GColorBlack,
              GTextAlignmentCenter);
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
    layer_mark_dirty(s_canvas);
  }
}

static void refresh_timer_callback(void *context) {
  request_refresh();
  s_refresh_timer = app_timer_register(REFRESH_INTERVAL_MS, refresh_timer_callback, NULL);
}

static void select_click(ClickRecognizerRef recognizer, void *context) { request_refresh(); }

static void click_config(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click);
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
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
