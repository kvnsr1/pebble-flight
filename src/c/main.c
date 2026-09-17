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
  char next_refresh[24];
  char aircraft_model[28];
  char aircraft_number[16];
  char aircraft_first_flight[20];
  char aircraft_leg_1[24];
  char aircraft_leg_1_status[24];
  char aircraft_leg_2[24];
  char aircraft_leg_2_status[24];
  char booking_code[16];
  char seat_number[8];
  char seat_position[12];
  char error[84];
  int status_level;
  int aircraft_leg_1_level;
  int aircraft_leg_2_level;
  bool loading;
} FlightData;

static Window *s_window;
static Layer *s_canvas;
static AppTimer *s_refresh_timer;
static AppTimer *s_loading_timer;
static FlightData s_flight;
static int s_page;
static int s_loading_frame;

#define REFRESH_INTERVAL_MS (5 * 60 * 1000)

static GColor status_color_for_level(int level) {
  switch (level) {
    case 0: return GColorIslamicGreen;
    case 1: return GColorChromeYellow;
    case 2: return GColorOrange;
    case 3: return GColorRed;
    default: return GColorDarkCandyAppleRed;
  }
}

static GColor status_color(void) { return status_color_for_level(s_flight.status_level); }

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
  for (int i = 0; i < 5; i++) {
    graphics_context_set_fill_color(ctx, i == s_page ? GColorBlue : GColorLightGray);
    graphics_fill_circle(ctx, GPoint(bounds.size.w - 4, 91 + i * 11), i == s_page ? 3 : 2);
  }
}

static void format_watch_time(char *buffer, size_t size) {
  time_t now = time(NULL);
  struct tm *local = localtime(&now);
  strftime(buffer, size, clock_is_24h_style() ? "%H:%M" : "%I:%M", local);
  if (!clock_is_24h_style() && buffer[0] == '0') {
    memmove(buffer, buffer + 1, strlen(buffer));
  }
}

static void draw_plane(GContext *ctx, GPoint center) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(center.x - 6, center.y - 1, 12, 3), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(center.x - 1, center.y - 5, 3, 11), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(center.x - 5, center.y - 3, 2, 7), 0, GCornerNone);
}

static void draw_flight_header(GContext *ctx, GRect bounds) {
  graphics_context_set_fill_color(ctx, GColorCobaltBlue);
  graphics_fill_rect(ctx, GRect(0, 0, bounds.size.w, 96), 0, GCornerNone);
  char clock_text[8];
  format_watch_time(clock_text, sizeof(clock_text));
  draw_text(ctx, clock_text, GRect(0, 1, bounds.size.w, 18),
            fonts_get_system_font(FONT_KEY_GOTHIC_14), GColorWhite,
            GTextAlignmentCenter);

  int route_x[] = {52, 64, 77, 90, 103, 116, 129, 142, 148};
  int route_y[] = {37, 31, 27, 24, 24, 26, 30, 35, 37};
  graphics_context_set_fill_color(ctx, GColorWhite);
  for (int i = 0; i < 9; i++) {
    graphics_fill_circle(ctx, GPoint(route_x[i], route_y[i]), (i == 0 || i == 8) ? 4 : 1);
  }
  draw_plane(ctx, GPoint(s_flight.loading ? 72 + s_loading_frame * 5 : 100, 25));
  draw_text(ctx, s_flight.flight, GRect(0, 40, bounds.size.w, 18),
            fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GColorWhite,
            GTextAlignmentCenter);
  char route[24];
  snprintf(route, sizeof(route), "%s  >  %s", s_flight.origin, s_flight.destination);
  draw_text(ctx, route, GRect(8, 55, bounds.size.w - 16, 28),
            fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GColorWhite,
            GTextAlignmentCenter);
  draw_text(ctx, s_flight.date, GRect(0, 79, bounds.size.w, 17),
            fonts_get_system_font(FONT_KEY_GOTHIC_14), GColorWhite,
            GTextAlignmentCenter);
}

static void draw_separator(GContext *ctx, int y, int width) {
  graphics_context_set_stroke_color(ctx, GColorLightGray);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, GPoint(5, y), GPoint(width - 8, y));
}

static void draw_refresh_footer(GContext *ctx, GRect bounds) {
  draw_separator(ctx, 197, bounds.size.w);
  if (s_flight.error[0]) {
    draw_text(ctx, s_flight.error, GRect(7, 199, bounds.size.w - 14, 29),
              fonts_get_system_font(FONT_KEY_GOTHIC_14), GColorRed,
              GTextAlignmentCenter);
    return;
  }
  char updated[32];
  snprintf(updated, sizeof(updated), s_flight.loading ? "Updating flight..." : "Updated %s", s_flight.updated);
  char next[40];
  snprintf(next, sizeof(next), "Next auto %s", s_flight.next_refresh);
  draw_text(ctx, updated, GRect(7, 198, bounds.size.w - 14, 16),
            fonts_get_system_font(FONT_KEY_GOTHIC_14), GColorDarkGray,
            GTextAlignmentCenter);
  draw_text(ctx, next, GRect(7, 212, bounds.size.w - 14, 16),
            fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GColorBlue,
            GTextAlignmentCenter);
}

static void draw_aircraft_image(GContext *ctx, GPoint center, GColor color) {
  graphics_context_set_fill_color(ctx, color);
  graphics_fill_rect(ctx, GRect(center.x - 2, center.y - 15, 5, 30), 0, GCornerNone);
  graphics_fill_circle(ctx, GPoint(center.x, center.y - 14), 3);
  graphics_fill_rect(ctx, GRect(center.x - 18, center.y - 2, 37, 5), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(center.x - 12, center.y - 6, 25, 5), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(center.x - 8, center.y + 10, 17, 4), 0, GCornerNone);
}

static void draw_timeline_row(GContext *ctx, int y, const char *route,
                              const char *status, int level, bool current) {
  graphics_context_set_fill_color(ctx, status_color_for_level(level));
  graphics_fill_circle(ctx, GPoint(17, y + 8), current ? 5 : 4);
  draw_text(ctx, route, GRect(29, y, 83, 18),
            fonts_get_system_font(current ? FONT_KEY_GOTHIC_14_BOLD : FONT_KEY_GOTHIC_14),
            GColorBlack, GTextAlignmentLeft);
  draw_text(ctx, status, GRect(111, y, 77, 18),
            fonts_get_system_font(FONT_KEY_GOTHIC_14),
            status_color_for_level(level), GTextAlignmentRight);
}

static void draw_aircraft_page(GContext *ctx, GRect bounds) {
  graphics_context_set_fill_color(ctx, GColorCobaltBlue);
  graphics_fill_rect(ctx, GRect(0, 0, bounds.size.w, 70), 0, GCornerNone);
  draw_text(ctx, "AIRCRAFT", GRect(8, 5, 92, 22),
            fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GColorWhite,
            GTextAlignmentLeft);
  draw_aircraft_image(ctx, GPoint(151, 35), GColorWhite);

  draw_text(ctx, "MODEL", GRect(7, 75, 55, 18),
            fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GColorDarkGray,
            GTextAlignmentLeft);
  draw_text(ctx, s_flight.aircraft_model, GRect(62, 73, 126, 22),
            fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GColorBlack,
            GTextAlignmentRight);
  draw_separator(ctx, 96, bounds.size.w);
  draw_text(ctx, "NUMBER", GRect(7, 99, 62, 18),
            fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GColorDarkGray,
            GTextAlignmentLeft);
  draw_text(ctx, s_flight.aircraft_number, GRect(69, 97, 119, 22),
            fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GColorBlue,
            GTextAlignmentRight);
  draw_separator(ctx, 121, bounds.size.w);
  draw_text(ctx, "FIRST FLIGHT", GRect(7, 124, 90, 18),
            fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GColorDarkGray,
            GTextAlignmentLeft);
  draw_text(ctx, s_flight.aircraft_first_flight, GRect(97, 122, 91, 22),
            fonts_get_system_font(FONT_KEY_GOTHIC_14), GColorBlack,
            GTextAlignmentRight);

  draw_text(ctx, "INCOMING LEGS", GRect(7, 146, 130, 18),
            fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GColorBlack,
            GTextAlignmentLeft);
  graphics_context_set_stroke_color(ctx, GColorLightGray);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_line(ctx, GPoint(17, 165), GPoint(17, 219));
  draw_timeline_row(ctx, 162, s_flight.aircraft_leg_2,
                    s_flight.aircraft_leg_2_status, s_flight.aircraft_leg_2_level, false);
  draw_timeline_row(ctx, 184, s_flight.aircraft_leg_1,
                    s_flight.aircraft_leg_1_status, s_flight.aircraft_leg_1_level, false);
  char current_route[24];
  snprintf(current_route, sizeof(current_route), "%s > %s", s_flight.origin, s_flight.destination);
  draw_timeline_row(ctx, 206, current_route, s_flight.status, s_flight.status_level, true);
  draw_page_dots(ctx, bounds);
}

static void draw_ticket_icon(GContext *ctx, GRect rect) {
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_round_rect(ctx, rect, 4);
  int divider = rect.origin.x + 43;
  for (int y = rect.origin.y + 4; y < rect.origin.y + rect.size.h - 3; y += 5) {
    graphics_draw_line(ctx, GPoint(divider, y), GPoint(divider, y + 2));
  }
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, GPoint(rect.origin.x, rect.origin.y + rect.size.h / 2), 3);
  graphics_fill_circle(ctx, GPoint(rect.origin.x + rect.size.w, rect.origin.y + rect.size.h / 2), 3);

  GPoint plane = GPoint(rect.origin.x + 22, rect.origin.y + 20);
  graphics_context_set_fill_color(ctx, GColorBlue);
  graphics_fill_rect(ctx, GRect(plane.x - 12, plane.y - 1, 24, 3), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(plane.x - 1, plane.y - 8, 3, 17), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(plane.x - 7, plane.y - 5, 3, 11), 0, GCornerNone);

  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_draw_rect(ctx, GRect(rect.origin.x + 49, rect.origin.y + 6, 11, 11));
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(rect.origin.x + 52, rect.origin.y + 9, 5, 5), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(rect.origin.x + 49, rect.origin.y + 23, 13, 2), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(rect.origin.x + 49, rect.origin.y + 28, 13, 2), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(rect.origin.x + 49, rect.origin.y + 33, 13, 2), 0, GCornerNone);
}

static void draw_seat(GContext *ctx, int x, bool selected) {
  GColor fill = selected ? GColorOxfordBlue : GColorLightGray;
  graphics_context_set_fill_color(ctx, fill);
  graphics_fill_rect(ctx, GRect(x, 132, 22, 29), 5, GCornersTop);
  graphics_fill_rect(ctx, GRect(x, 162, 22, 14), 2, GCornersBottom);
  graphics_fill_rect(ctx, GRect(x - 3, 157, 4, 17), 1, GCornersAll);
  graphics_fill_rect(ctx, GRect(x + 21, 157, 4, 17), 1, GCornersAll);
  graphics_fill_rect(ctx, GRect(x + 3, 175, 4, 7), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(x + 16, 175, 4, 7), 0, GCornerNone);
  graphics_context_set_stroke_color(ctx, selected ? GColorBlack : GColorDarkGray);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_round_rect(ctx, GRect(x, 132, 22, 44), 5);
  graphics_draw_line(ctx, GPoint(x + 4, 158), GPoint(x + 18, 158));
}

static void draw_seat_map(GContext *ctx) {
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_round_rect(ctx, GRect(7, 136, 24, 40), 11);
  graphics_context_set_stroke_color(ctx, GColorBlue);
  graphics_draw_round_rect(ctx, GRect(11, 140, 16, 32), 8);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, GPoint(13, 164), GPoint(24, 147));
  graphics_draw_line(ctx, GPoint(16, 169), GPoint(26, 154));

  const char *labels[] = {"WINDOW", "MIDDLE", "AISLE"};
  const int xs[] = {39, 68, 97};
  for (int i = 0; i < 3; i++) {
    bool selected = strcmp(s_flight.seat_position, labels[i]) == 0;
    draw_seat(ctx, xs[i], selected);
  }
}

static void draw_booking_page(GContext *ctx, GRect bounds) {
  graphics_context_set_fill_color(ctx, GColorCobaltBlue);
  graphics_fill_rect(ctx, GRect(0, 0, bounds.size.w, 54), 0, GCornerNone);
  draw_text(ctx, "BOOKING", GRect(8, 7, bounds.size.w - 16, 24),
            fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GColorWhite,
            GTextAlignmentCenter);
  draw_text(ctx, s_flight.flight, GRect(8, 28, bounds.size.w - 16, 20),
            fonts_get_system_font(FONT_KEY_GOTHIC_14), GColorWhite,
            GTextAlignmentCenter);

  draw_ticket_icon(ctx, GRect(8, 66, 68, 40));
  draw_text(ctx, "BOOKING CODE", GRect(84, 63, 104, 18),
            fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GColorDarkGray,
            GTextAlignmentLeft);
  draw_text(ctx, s_flight.booking_code, GRect(84, 80, 104, 28),
            fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GColorBlue,
            GTextAlignmentLeft);
  draw_separator(ctx, 116, bounds.size.w);

  draw_seat_map(ctx);
  draw_text(ctx, "SEAT", GRect(132, 127, 56, 18),
            fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GColorDarkGray,
            GTextAlignmentLeft);
  draw_text(ctx, s_flight.seat_number, GRect(130, 143, 58, 34),
            fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK), GColorBlue,
            GTextAlignmentLeft);
  draw_text(ctx, s_flight.seat_position, GRect(128, 177, 60, 20),
            fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GColorBlack,
            GTextAlignmentLeft);
  draw_text(ctx, "Private • stored on phone", GRect(8, 208, bounds.size.w - 16, 18),
            fonts_get_system_font(FONT_KEY_GOTHIC_14), GColorDarkGray,
            GTextAlignmentCenter);
  draw_page_dots(ctx, bounds);
}

static void canvas_update(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, b, 0, GCornerNone);

  if (s_page == 0) {
    draw_flight_header(ctx, b);
    draw_text(ctx, "STATUS", GRect(6, 100, 70, 20),
              fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GColorBlack,
              GTextAlignmentLeft);
    draw_text(ctx, s_flight.status, GRect(72, 100, b.size.w - 82, 20),
              fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), status_color(),
              GTextAlignmentRight);
    draw_separator(ctx, 122, b.size.w);

    draw_text(ctx, "DEPART", GRect(6, 125, 88, 18),
              fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GColorBlack,
              GTextAlignmentLeft);
    draw_text(ctx, "ARRIVE", GRect(101, 125, 88, 18),
              fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GColorBlack,
              GTextAlignmentRight);
    draw_text(ctx, s_flight.departure_time, GRect(6, 141, 91, 27),
              fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GColorBlue,
              GTextAlignmentLeft);
    draw_text(ctx, s_flight.arrival_time, GRect(101, 141, 88, 27),
              fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GColorBlue,
              GTextAlignmentRight);
    draw_separator(ctx, 169, b.size.w);

    char departure_place[36];
    char arrival_place[36];
    snprintf(departure_place, sizeof(departure_place), "Gate %s  T%s",
             s_flight.departure_gate, s_flight.departure_terminal);
    snprintf(arrival_place, sizeof(arrival_place), "Gate %s  T%s",
             s_flight.arrival_gate, s_flight.arrival_terminal);
    draw_text(ctx, departure_place, GRect(6, 173, 92, 20),
              fonts_get_system_font(FONT_KEY_GOTHIC_14), GColorBlack,
              GTextAlignmentLeft);
    draw_text(ctx, arrival_place, GRect(100, 173, 89, 20),
              fonts_get_system_font(FONT_KEY_GOTHIC_14), GColorBlack,
              GTextAlignmentRight);
    draw_refresh_footer(ctx, b);
    draw_page_dots(ctx, b);
  } else if (s_page <= 2) {
    bool departure = s_page == 1;
    const char *airport = departure ? s_flight.origin : s_flight.destination;
    const char *time = departure ? s_flight.departure_time : s_flight.arrival_time;
    const char *terminal = departure ? s_flight.departure_terminal : s_flight.arrival_terminal;
    const char *gate = departure ? s_flight.departure_gate : s_flight.arrival_gate;
    graphics_context_set_fill_color(ctx, GColorCobaltBlue);
    graphics_fill_rect(ctx, GRect(0, 0, b.size.w, 70), 0, GCornerNone);
    char clock_text[8];
    format_watch_time(clock_text, sizeof(clock_text));
    draw_text(ctx, clock_text, GRect(0, 1, b.size.w, 18),
              fonts_get_system_font(FONT_KEY_GOTHIC_14), GColorWhite,
              GTextAlignmentCenter);
    draw_text(ctx, departure ? "DEPARTURE" : "ARRIVAL", GRect(8, 20, b.size.w - 16, 20),
              fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GColorWhite,
              GTextAlignmentCenter);
    draw_text(ctx, airport, GRect(8, 36, b.size.w - 16, 34),
              fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK), GColorWhite,
              GTextAlignmentCenter);
    draw_text(ctx, time, GRect(8, 76, b.size.w - 16, 38),
              fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK), GColorBlue,
              GTextAlignmentCenter);
    draw_text(ctx, s_flight.date, GRect(8, 108, b.size.w - 16, 18),
              fonts_get_system_font(FONT_KEY_GOTHIC_14), GColorDarkGray,
              GTextAlignmentCenter);
    draw_separator(ctx, 130, b.size.w);
    draw_text(ctx, "GATE", GRect(8, 134, 80, 22),
              fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GColorBlack,
              GTextAlignmentLeft);
    draw_text(ctx, gate, GRect(100, 134, 88, 22),
              fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GColorBlack,
              GTextAlignmentRight);
    draw_separator(ctx, 158, b.size.w);
    draw_text(ctx, "TERMINAL", GRect(8, 162, 80, 22),
              fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GColorBlack,
              GTextAlignmentLeft);
    draw_text(ctx, terminal, GRect(100, 162, 88, 22),
              fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GColorBlack,
              GTextAlignmentRight);
    draw_refresh_footer(ctx, b);
    draw_page_dots(ctx, b);
  } else if (s_page == 3) {
    draw_aircraft_page(ctx, b);
  } else {
    draw_booking_page(ctx, b);
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

static void request_aircraft_details(void) {
  DictionaryIterator *out;
  if (app_message_outbox_begin(&out) == APP_MSG_OK) {
    dict_write_uint8(out, MESSAGE_KEY_REQUEST_AIRCRAFT_DETAILS, 1);
    app_message_outbox_send();
  }
}

static void change_page(int delta) {
  s_page = (s_page + delta + 5) % 5;
  layer_mark_dirty(s_canvas);
  if (s_page == 3) { request_aircraft_details(); }
}

static void up_click(ClickRecognizerRef recognizer, void *context) { change_page(-1); }
static void down_click(ClickRecognizerRef recognizer, void *context) { change_page(1); }

static void request_refresh(bool is_manual) {
  DictionaryIterator *out;
  if (app_message_outbox_begin(&out) == APP_MSG_OK) {
    dict_write_uint8(out, MESSAGE_KEY_REQUEST_REFRESH, is_manual ? 1 : 2);
    app_message_outbox_send();
    s_flight.loading = true;
    s_flight.error[0] = '\0';
    start_loading_animation();
    layer_mark_dirty(s_canvas);
  }
}

static void refresh_timer_callback(void *context) {
  request_refresh(false);
  s_refresh_timer = app_timer_register(REFRESH_INTERVAL_MS, refresh_timer_callback, NULL);
}

static void select_click(ClickRecognizerRef recognizer, void *context) { request_refresh(true); }

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

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if (s_canvas) { layer_mark_dirty(s_canvas); }
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
  copy_tuple(iter, MESSAGE_KEY_NEXT_REFRESH_AT, s_flight.next_refresh, sizeof(s_flight.next_refresh));
  copy_tuple(iter, MESSAGE_KEY_AIRCRAFT_MODEL, s_flight.aircraft_model, sizeof(s_flight.aircraft_model));
  copy_tuple(iter, MESSAGE_KEY_AIRCRAFT_NUMBER, s_flight.aircraft_number, sizeof(s_flight.aircraft_number));
  copy_tuple(iter, MESSAGE_KEY_AIRCRAFT_FIRST_FLIGHT, s_flight.aircraft_first_flight, sizeof(s_flight.aircraft_first_flight));
  copy_tuple(iter, MESSAGE_KEY_AIRCRAFT_LEG_1, s_flight.aircraft_leg_1, sizeof(s_flight.aircraft_leg_1));
  copy_tuple(iter, MESSAGE_KEY_AIRCRAFT_LEG_1_STATUS, s_flight.aircraft_leg_1_status, sizeof(s_flight.aircraft_leg_1_status));
  copy_tuple(iter, MESSAGE_KEY_AIRCRAFT_LEG_2, s_flight.aircraft_leg_2, sizeof(s_flight.aircraft_leg_2));
  copy_tuple(iter, MESSAGE_KEY_AIRCRAFT_LEG_2_STATUS, s_flight.aircraft_leg_2_status, sizeof(s_flight.aircraft_leg_2_status));
  copy_tuple(iter, MESSAGE_KEY_BOOKING_CODE, s_flight.booking_code, sizeof(s_flight.booking_code));
  copy_tuple(iter, MESSAGE_KEY_SEAT_NUMBER, s_flight.seat_number, sizeof(s_flight.seat_number));
  copy_tuple(iter, MESSAGE_KEY_SEAT_POSITION, s_flight.seat_position, sizeof(s_flight.seat_position));
  copy_tuple(iter, MESSAGE_KEY_ERROR_MESSAGE, s_flight.error, sizeof(s_flight.error));
  Tuple *level = dict_find(iter, MESSAGE_KEY_STATUS_LEVEL);
  Tuple *leg_1_level = dict_find(iter, MESSAGE_KEY_AIRCRAFT_LEG_1_LEVEL);
  Tuple *leg_2_level = dict_find(iter, MESSAGE_KEY_AIRCRAFT_LEG_2_LEVEL);
  Tuple *loading = dict_find(iter, MESSAGE_KEY_IS_LOADING);
  if (level) { s_flight.status_level = (int)level->value->int32; }
  if (leg_1_level) { s_flight.aircraft_leg_1_level = (int)leg_1_level->value->int32; }
  if (leg_2_level) { s_flight.aircraft_leg_2_level = (int)leg_2_level->value->int32; }
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
  snprintf(s_flight.updated, sizeof(s_flight.updated), "--");
  snprintf(s_flight.next_refresh, sizeof(s_flight.next_refresh), "--");
  snprintf(s_flight.aircraft_model, sizeof(s_flight.aircraft_model), "--");
  snprintf(s_flight.aircraft_number, sizeof(s_flight.aircraft_number), "--");
  snprintf(s_flight.aircraft_first_flight, sizeof(s_flight.aircraft_first_flight), "Unavailable");
  snprintf(s_flight.aircraft_leg_1, sizeof(s_flight.aircraft_leg_1), "--");
  snprintf(s_flight.aircraft_leg_1_status, sizeof(s_flight.aircraft_leg_1_status), "--");
  snprintf(s_flight.aircraft_leg_2, sizeof(s_flight.aircraft_leg_2), "--");
  snprintf(s_flight.aircraft_leg_2_status, sizeof(s_flight.aircraft_leg_2_status), "--");
  snprintf(s_flight.booking_code, sizeof(s_flight.booking_code), "--");
  snprintf(s_flight.seat_number, sizeof(s_flight.seat_number), "--");
  snprintf(s_flight.seat_position, sizeof(s_flight.seat_position), "--");
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){.load = window_load, .unload = window_unload});
  window_set_click_config_provider(s_window, click_config);
  window_stack_push(s_window, true);
  app_message_register_inbox_received(inbox_received);
  app_message_open(1024, 128);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  s_refresh_timer = app_timer_register(REFRESH_INTERVAL_MS, refresh_timer_callback, NULL);
}

static void deinit(void) {
  if (s_refresh_timer) { app_timer_cancel(s_refresh_timer); }
  if (s_loading_timer) { app_timer_cancel(s_loading_timer); }
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
