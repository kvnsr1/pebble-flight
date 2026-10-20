#include "../prototype_config.h"
#if PEBBLE_UI_PROTOTYPE
#include <pebble.h>
#include "flight_model.h"
#include "navigation.h"
#include "pebble_plane.h"
#include "booking_icons.h"
#include "airline_icons.h"
#include "live_data.h"
#include "motion.h"

static Window *s_window;
static Layer *s_canvas;
static PrototypeNavigation s_nav = { .page = PAGE_LIST };
typedef enum { MOVE_NONE, MOVE_SELECTION, MOVE_PAGE, MOVE_OPEN, MOVE_CLOSE, MOVE_NOTE } Move;
static Layer *s_in_clip, *s_out_clip, *s_outgoing, *s_indicators, *s_hero;
static Animation *s_move_anim, *s_progress_anim;
static AppTimer *s_emphasis_timer, *s_activity_timer;
static PrototypeNavigation s_before;
static Move s_move;
static int s_move_p = MOTION_UNIT, s_progress_p = MOTION_UNIT;
static int s_progress_from, s_progress_to;
static unsigned s_emphasis, s_activity_frame;
static bool s_drawing_old, s_before_aircraft, s_visible;
enum { CH_STATUS = 1, CH_DEP_TIME = 2, CH_ARR_TIME = 4, CH_DEP_TERMINAL = 8,
       CH_GATE = 16, CH_ARR_TERMINAL = 32, CH_BAGGAGE = 64, CH_BOOKING = 128, CH_SEAT = 256 };
static void motion_stop(void);
static void feedback_stop(void);
static void activity_sync(void);
static void remember_flight(void);
static void draw_emphasis(GContext *ctx);
static void motion_start(PrototypeNavigation before, bool before_aircraft);
#if PEBBLE_MOCK_DATA && PEBBLE_MOTION_DEMO
static Flight s_demo_flight;
static size_t s_demo_index, s_demo_target;
static bool s_demo_valid, s_demo_loading;
static AppTimer *s_demo_timer;
static void demo_refresh(void);
#endif
#if !PEBBLE_MOCK_DATA
static Window *s_actions_window;
static MenuLayer *s_actions_menu;
static bool s_aircraft;
static char s_selected_key[LIVE_TEXT_SIZE];
#endif
static bool aircraft_showing(void) {
#if !PEBBLE_MOCK_DATA
  return s_aircraft;
#else
  return false;
#endif
}
static size_t data_count(void) {
#if PEBBLE_MOCK_DATA
  return mock_flight_count();
#else
  return live_flight_count();
#endif
}
static const Flight *data_at(size_t index) {
#if PEBBLE_MOCK_DATA
#if PEBBLE_MOTION_DEMO
  if (s_demo_valid && index == s_demo_index) { return &s_demo_flight; }
#endif
  return mock_flight_at(index);
#else
  return live_flight_at(index);
#endif
}
static const char *source_label(void) {
#if PEBBLE_MOCK_DATA
#if PEBBLE_MOTION_DEMO
  if (s_demo_loading && s_demo_target == s_nav.selected) { return "WAIT"; }
#endif
  return "MOCK";
#else
  const NativeFlight *m=live_metadata_at(s_nav.selected);
  if (!connection_service_peek_pebble_app_connection()) { return "OFFLINE"; }
  if (live_global_error()[0] || (m && m->text[TX_ERROR][0])) { return "SAVED"; }
  if (m && m->loading) { return "WAIT"; }
  return m && !m->stale ? "LIVE" : "SAVED";
#endif
}

// Every color is from Pebble's 64-color palette. No gradients or phone UI.
static GColor background(void) { return GColorDarkGreen; }
static GColor accent(void) { return GColorMalachite; }
static GFont font(const char *key) { return fonts_get_system_font(key); }
static void text(GContext *ctx, const char *value, int x, int y, int w, int h,
                 const char *key, GColor color, GTextAlignment align) {
  graphics_context_set_text_color(ctx, color);
  graphics_draw_text(ctx, value, font(key), GRect(x,y,w,h),
                     GTextOverflowModeTrailingEllipsis, align, NULL);
}
static void label(GContext *ctx, const char *value, int x, int y, int w) {
  text(ctx, value, x,y,w,18, FONT_KEY_GOTHIC_14_BOLD, GColorWhite, GTextAlignmentLeft);
}
static void fill(GContext *ctx, GRect rect, GColor color) {
  graphics_context_set_fill_color(ctx, color);
  graphics_fill_rect(ctx, rect, 0, GCornerNone);
}
static GColor status_color(const Flight *flight) {
  if (flight->status == FLIGHT_CANCELED || flight->status == FLIGHT_UNKNOWN || flight->status == FLIGHT_DIVERTED) { return GColorWhite; }
  if (flight->status == FLIGHT_DELAYED) { return GColorYellow; }
  return accent();
}
static const char *status_text(const Flight *flight, char *buffer, size_t size) {
  if (flight->status==FLIGHT_DELAYED && flight->departure.time.delay_known &&
      flight->departure.time.delay_minutes>0) {
    snprintf(buffer,size,"Delayed +%d min",flight->departure.time.delay_minutes);
    return buffer;
  }
  return flight_status_label(flight->status);
}
static void status_icon(GContext *ctx, FlightStatus status, GPoint p, GColor color) {
  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(ctx, 2);
  if (status == FLIGHT_CANCELED || status == FLIGHT_DIVERTED) {
    graphics_draw_line(ctx, GPoint(p.x-4,p.y-4), GPoint(p.x+4,p.y+4));
    graphics_draw_line(ctx, GPoint(p.x+4,p.y-4), GPoint(p.x-4,p.y+4));
  } else if (status == FLIGHT_LANDED) {
    graphics_draw_line(ctx, GPoint(p.x-5,p.y), GPoint(p.x-1,p.y+4));
    graphics_draw_line(ctx, GPoint(p.x-1,p.y+4), GPoint(p.x+5,p.y-4));
  } else if (status == FLIGHT_IN_FLIGHT || status == FLIGHT_BOARDING) {
    graphics_draw_line(ctx, GPoint(p.x-5,p.y), GPoint(p.x+5,p.y));
    graphics_draw_line(ctx, GPoint(p.x+1,p.y-4), GPoint(p.x+5,p.y));
    graphics_draw_line(ctx, GPoint(p.x+1,p.y+4), GPoint(p.x+5,p.y));
  } else {
    graphics_draw_circle(ctx, p, 5);
    graphics_draw_line(ctx, p, GPoint(p.x,p.y-3));
    graphics_draw_line(ctx, p, GPoint(p.x+3,p.y));
  }
}
static void page_indicator(GContext *ctx) {
  int active_y = 88 + s_nav.page * 10;
  if (s_move == MOVE_PAGE) {
    active_y = motion_mix(88 + s_before.page * 10, active_y, motion_ease(s_move_p));
  }
  for (int i=0; i<4; i++) {
    graphics_context_set_fill_color(ctx, GColorMayGreen);
    graphics_fill_circle(ctx, GPoint(194,88+i*10), 2);
  }
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, GPoint(194,active_y), 3);
  // Tiny physical-button cues; absent at the first and last pages.
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_stroke_width(ctx, 1);
  if (s_nav.page > PAGE_OVERVIEW) {
    graphics_draw_line(ctx, GPoint(191,68),GPoint(194,65));
    graphics_draw_line(ctx, GPoint(194,65),GPoint(197,68));
  }
  if (s_nav.page < PAGE_BOOKING) {
    graphics_draw_line(ctx, GPoint(191,130),GPoint(194,133));
    graphics_draw_line(ctx, GPoint(194,133),GPoint(197,130));
  }
}
static void draw_list(GContext *ctx) {
  fill(ctx, GRect(0,0,200,228), GColorBlack);
  text(ctx, "All flights",10,0,148,32,FONT_KEY_GOTHIC_28_BOLD,GColorWhite,GTextAlignmentLeft);
  char count[16];
  snprintf(count,sizeof(count),"%d / %d",s_nav.empty?0:(int)s_nav.selected+1,
           s_nav.empty?0:(int)data_count());
  text(ctx,count,151,10,39,18,FONT_KEY_GOTHIC_14,GColorWhite,GTextAlignmentRight);
  text(ctx,
#if PEBBLE_MOCK_DATA
       "MOCK  /  OCT 2026",
#else
       "YOUR FLIGHTS",
#endif
       10,30,178,18,FONT_KEY_GOTHIC_14,accent(),GTextAlignmentLeft);
  if (s_nav.empty) {
    int lift = s_move == MOVE_NOTE ? (s_move_p < 500 ? motion_mix(0,2,s_move_p*2) : motion_mix(2,0,(s_move_p-500)*2)) : 0;
    pebble_plane_draw(ctx,GPoint(98,84-lift));
    text(ctx,
#if PEBBLE_MOCK_DATA
         "No flights yet",
#else
         connection_service_peek_pebble_app_connection()?"No flights yet":"Phone offline",
#endif
         10,111,180,32,FONT_KEY_GOTHIC_24_BOLD,GColorWhite,GTextAlignmentCenter);
    text(ctx,
#if PEBBLE_MOCK_DATA
         "Your logged flights\nwill appear here.",
#else
         connection_service_peek_pebble_app_connection()?"Add flights in your\nPebble phone settings.":"Reconnect to show\nyour saved flights.",
#endif
         10,147,180,40,FONT_KEY_GOTHIC_18,GColorWhite,GTextAlignmentCenter);
    text(ctx,
#if PEBBLE_MOCK_DATA
         "Hold SELECT: restore demo",
#else
         live_global_error(),
#endif
         5,209,190,18,FONT_KEY_GOTHIC_14,accent(),GTextAlignmentCenter);
    return;
  }
  int row_shift = 0;
  int selection_y = 51 + (int)(s_nav.selected - s_nav.top) * 51;
  bool selection_move = s_move == MOVE_SELECTION && !s_drawing_old;
  if (selection_move) {
    int p = motion_ease(s_move_p);
    row_shift = motion_mix(((int)s_nav.top - (int)s_before.top) * 51, 0, p);
    selection_y = motion_mix(51 + ((int)s_before.selected - (int)s_before.top) * 51,
                            selection_y, p);
  }
  fill(ctx,GRect(0,selection_y,190,51),background());
  fill(ctx,GRect(0,selection_y,3,51),accent());
  for (int row=-1; row<=VISIBLE_FLIGHTS; row++) {
    if ((int)s_nav.top + row < 0) { continue; }
    size_t index=s_nav.top+row;
    const Flight *flight=data_at(index);
    if (!flight) { break; }
    int y=51+row*51+row_shift;
    bool selected=index==s_nav.selected;
    bool hero = (s_move == MOVE_OPEN || (s_move == MOVE_CLOSE && s_before.page == PAGE_OVERVIEW && !s_before.note)) && selected;
    if (!hero) { text(ctx,flight->number,10,y-2,100,24,FONT_KEY_GOTHIC_18_BOLD,GColorWhite,GTextAlignmentLeft); }
    text(ctx,flight->date,106,y+1,72,18,FONT_KEY_GOTHIC_14,GColorWhite,GTextAlignmentRight);
    char route[24];
    snprintf(route,sizeof(route),"%s > %s",flight->departure.code,flight->arrival.code);
    text(ctx,route,10,y+16,162,24,FONT_KEY_GOTHIC_18_BOLD,GColorWhite,GTextAlignmentLeft);
    GColor tone=status_color(flight);
    status_icon(ctx,flight->status,GPoint(15,y+43),tone);
    char status[40];
    text(ctx,status_text(flight,status,sizeof(status)),27,y+34,149,17,FONT_KEY_GOTHIC_14,tone,GTextAlignmentLeft);
    if (selected) {
      graphics_context_set_stroke_color(ctx,GColorWhite);
      graphics_context_set_stroke_width(ctx,2);
      graphics_draw_line(ctx,GPoint(179,selection_y+22),GPoint(183,selection_y+26));
      graphics_draw_line(ctx,GPoint(183,selection_y+26),GPoint(179,selection_y+30));
    }
  }
  // Fixed chrome masks the scrolling rows, using the original list geometry.
  fill(ctx,GRect(0,0,200,51),GColorBlack);
  text(ctx,"All flights",10,0,148,32,FONT_KEY_GOTHIC_28_BOLD,GColorWhite,GTextAlignmentLeft);
  text(ctx,count,151,10,39,18,FONT_KEY_GOTHIC_14,GColorWhite,GTextAlignmentRight);
  text(ctx,
#if PEBBLE_MOCK_DATA
       "MOCK  /  OCT 2026",
#else
       "YOUR FLIGHTS",
#endif
       10,30,178,18,FONT_KEY_GOTHIC_14,accent(),GTextAlignmentLeft);
  fill(ctx,GRect(0,204,200,24),GColorBlack);
  if (data_count()>VISIBLE_FLIGHTS) {
    fill(ctx,GRect(195,52,2,150),GColorDarkGray);
    int thumb_h=150*VISIBLE_FLIGHTS/(int)data_count();
    int thumb_y=52+(150-thumb_h)*(int)s_nav.top/((int)data_count()-VISIBLE_FLIGHTS);
    fill(ctx,GRect(194,thumb_y,4,thumb_h),GColorWhite);
  }
  const char *footer="Hold SELECT: empty demo";
#if !PEBBLE_MOCK_DATA
  const NativeFlight *metadata=live_metadata_at(s_nav.selected);
  footer=!connection_service_peek_pebble_app_connection()?"Offline / saved data":
    live_global_error()[0]?"Phone unavailable / saved data":
    metadata && metadata->loading?"Updating flight...":
    metadata && metadata->text[TX_ERROR][0]?"Saved data / SELECT for details":"Hold SELECT: actions";
#endif
  text(ctx,footer,5,209,190,18,FONT_KEY_GOTHIC_14,GColorLightGray,GTextAlignmentCenter);
}
static const char *overview_time(const FlightTime *time) {
  return time->kind==TIME_ACTUAL || time->kind==TIME_ESTIMATED ? time->current : time->scheduled;
}
static const char *time_heading(const FlightTime *time, bool departure) {
  if (time->kind==TIME_ACTUAL) { return departure?"DEP / ACTUAL":"ARR / ACTUAL"; }
  if (time->kind==TIME_ESTIMATED) { return departure?"DEP / EST":"ARR / EST"; }
  return departure?"DEP / SCHED":"ARR / SCHED";
}
static void draw_overview(GContext *ctx, const Flight *flight) {
  label(ctx,"OVERVIEW",10,1,120);
  text(ctx,source_label(),145,1,40,18,FONT_KEY_GOTHIC_14,GColorWhite,GTextAlignmentRight);
  // Retain a 116px outside diameter. The 8px stroke and aircraft marker use
  // the same 108px centerline circle, keeping the marker on the track.
  GRect ring=GRect(40,24,116,116);
  GRect track=grect_inset(ring,GEdgeInsets(4));
  int start=DEG_TO_TRIGANGLE(235);
  int sweep=DEG_TO_TRIGANGLE(250);
  int progress=flight_progress(flight);
  bool hero = s_move == MOVE_OPEN || (s_move == MOVE_CLOSE && s_before.page == PAGE_OVERVIEW && !s_before.note);
  bool show_labels = !hero || (s_move == MOVE_OPEN && s_move_p >= 600);
  int drawn_progress = progress;
  if (s_progress_anim && !s_drawing_old && progress >= 0) {
    drawn_progress = motion_mix(s_progress_from, s_progress_to, motion_ease(s_progress_p));
  }
  graphics_context_set_stroke_width(ctx,8);
  graphics_context_set_stroke_color(ctx,GColorMayGreen);
  graphics_draw_arc(ctx,track,GOvalScaleModeFitCircle,start,start+sweep);
  if (drawn_progress>0) {
    graphics_context_set_stroke_color(ctx,accent());
    graphics_draw_arc(ctx,track,GOvalScaleModeFitCircle,start,start+sweep*drawn_progress/100);
  }
  if (progress>=0) {
    GPoint marker=gpoint_from_polar(track,GOvalScaleModeFitCircle,start+sweep*drawn_progress/100);
    if (s_move == MOVE_OPEN && !s_drawing_old) {
      int p = s_move_p * MOTION_FLIGHT_MS / MOTION_PLANE_MS;
      if (p < MOTION_UNIT) { marker.y -= p < 500 ? motion_mix(0,2,p*2) : motion_mix(2,0,(p-500)*2); }
    }
    pebble_plane_progress_draw(ctx,marker,flight->status == FLIGHT_IN_FLIGHT);
    char percent[16];
    snprintf(percent,sizeof(percent),"%d%%",progress);
    if (show_labels) {
      text(ctx,percent,56,40,84,28,FONT_KEY_GOTHIC_24_BOLD,GColorWhite,GTextAlignmentCenter);
    }
  } else {
    if (show_labels) {
      if (progress==-1) { status_icon(ctx,FLIGHT_CANCELED,GPoint(98,39),GColorWhite); }
      text(ctx,progress==-1?"NO JOURNEY":"PROGRESS",56,progress==-1?44:29,84,18,FONT_KEY_GOTHIC_14,GColorWhite,GTextAlignmentCenter);
      if (progress==-2) { text(ctx,"UNKNOWN",56,43,84,18,FONT_KEY_GOTHIC_14,GColorWhite,GTextAlignmentCenter); }
    }
  }
  if (show_labels && !airline_icon_draw(ctx,flight->number,GPoint(86,66))) {
    pebble_plane_draw(ctx,GPoint(98,78));
  }
  if (!(s_move == MOVE_OPEN || (s_move == MOVE_CLOSE && s_before.page == PAGE_OVERVIEW && !s_before.note))) {
    text(ctx,flight->number,51,91,94,25,FONT_KEY_GOTHIC_18_BOLD,GColorWhite,GTextAlignmentCenter);
  }
  // Let the shared flight number clear the status line before revealing it.
  if (show_labels) {
    status_icon(ctx,flight->status,GPoint(23,136),status_color(flight));
    char status[40];
    text(ctx,status_text(flight,status,sizeof(status)),35,123,145,25,FONT_KEY_GOTHIC_18_BOLD,status_color(flight),GTextAlignmentCenter);
  }
  label(ctx,time_heading(&flight->departure.time,true),10,151,90);
  label(ctx,time_heading(&flight->arrival.time,false),106,151,84);
  text(ctx,flight->departure.code,10,163,86,34,FONT_KEY_GOTHIC_28_BOLD,GColorWhite,GTextAlignmentLeft);
  text(ctx,flight->arrival.code,106,163,84,34,FONT_KEY_GOTHIC_28_BOLD,GColorWhite,GTextAlignmentLeft);
  text(ctx,overview_time(&flight->departure.time),10,193,86,25,FONT_KEY_GOTHIC_18_BOLD,accent(),GTextAlignmentLeft);
  text(ctx,overview_time(&flight->arrival.time),106,193,84,25,FONT_KEY_GOTHIC_18_BOLD,accent(),GTextAlignmentLeft);
  char zone[32];
  snprintf(zone,sizeof(zone),"%s %s",flight->departure.time.zone,flight->departure.time.date);
  text(ctx,zone,10,213,90,15,FONT_KEY_GOTHIC_14,GColorWhite,GTextAlignmentLeft);
  snprintf(zone,sizeof(zone),"%s %s",flight->arrival.time.zone,flight->arrival.time.date);
  text(ctx,zone,106,213,90,15,FONT_KEY_GOTHIC_14,GColorWhite,GTextAlignmentLeft);
}
static void draw_airport(GContext *ctx, const Flight *flight, bool departure) {
  const FlightAirport *airport=departure?&flight->departure:&flight->arrival;
  const FlightTime *time=&airport->time;
  char heading[40];
  snprintf(heading,sizeof(heading),"%s / %s",flight->number,departure?"DEPARTURE":"ARRIVAL");
  label(ctx,heading,10,1,178);
  text(ctx,airport->code,9,17,178,50,FONT_KEY_BITHAM_42_BOLD,GColorWhite,GTextAlignmentLeft);
  text(ctx,airport->name,10,65,177,34,FONT_KEY_GOTHIC_14,GColorWhite,GTextAlignmentLeft);
  bool date_change=time->scheduled_date && time->scheduled_date[0] &&
    strcmp(time->scheduled_date,"Unknown") && strcmp(time->scheduled_date,time->date);
  bool zone_change=time->scheduled_zone && time->scheduled_zone[0];
  char schedule_label[40];
  snprintf(schedule_label,sizeof(schedule_label),date_change?"SCHED %s":"SCHEDULED",time->scheduled_date);
  if (zone_change) { snprintf(schedule_label,sizeof(schedule_label),"SCHED %s",time->scheduled_zone); }
  label(ctx,schedule_label,10,103,94);
  text(ctx,time->scheduled,109,96,78,30,FONT_KEY_GOTHIC_24_BOLD,GColorWhite,GTextAlignmentRight);
  const char *kind=time->kind==TIME_ACTUAL?"ACTUAL":time->kind==TIME_ESTIMATED?"ESTIMATED":
                   time->kind==TIME_UNKNOWN?"NOT REPORTED":"UNAVAILABLE";
  const char *value=time->kind==TIME_UNKNOWN?"Unknown":time->kind==TIME_UNAVAILABLE?"N/A":time->current;
  char current_label[32];
  snprintf(current_label,sizeof(current_label),date_change?"%s %s":"%s",
           date_change?(time->kind==TIME_ACTUAL?"ACT":"EST"):kind,time->date);
  if (zone_change) { snprintf(current_label,sizeof(current_label),"%s %s",time->kind==TIME_ACTUAL?"ACT":"EST",time->zone); }
  label(ctx,current_label,10,130,94);
  text(ctx,value,106,123,81,30,strlen(value)>5?FONT_KEY_GOTHIC_18_BOLD:FONT_KEY_GOTHIC_24_BOLD,
       accent(),GTextAlignmentRight);
  int offset=time->utc_offset_minutes;
  char zone[48];
  snprintf(zone,sizeof(zone),"%s / %s / UTC%c%d",time->date,time->zone,offset<0?'-':'+',abs(offset)/60);
  if (offset==2000) { snprintf(zone,sizeof(zone),"%s / Timezone unknown",time->date); }
  else if (abs(offset)%60) {
    size_t len=strlen(zone);
    snprintf(zone+len,sizeof(zone)-len,":%02d",abs(offset)%60);
  }
  if (zone_change && date_change) {
    char utc[16];
    snprintf(utc,sizeof(utc),"UTC%c%d",offset<0?'-':'+',abs(offset)/60);
    if (abs(offset)%60) { size_t n=strlen(utc); snprintf(utc+n,sizeof(utc)-n,":%02d",abs(offset)%60); }
    snprintf(zone,sizeof(zone),"%s>%s %s",time->scheduled_date,time->date,utc);
  }
  text(ctx,zone,10,153,178,18,FONT_KEY_GOTHIC_14,GColorWhite,GTextAlignmentLeft);
  char delay[40];
  GColor tone=accent();
  if (flight->status==FLIGHT_CANCELED) { snprintf(delay,sizeof(delay),"Canceled"); tone=GColorWhite; }
  else if (!time->delay_known) { snprintf(delay,sizeof(delay),"Timing unknown"); tone=GColorWhite; }
  else if (!time->delay_minutes) { snprintf(delay,sizeof(delay),"On time"); }
  else {
    snprintf(delay,sizeof(delay),"%d min %s",abs(time->delay_minutes),time->delay_minutes<0?"early":"late");
    if (time->delay_minutes>0) { tone=GColorYellow; }
  }
  status_icon(ctx,flight->status==FLIGHT_CANCELED?FLIGHT_CANCELED:
                   time->delay_known && time->delay_minutes<=0?FLIGHT_LANDED:FLIGHT_DELAYED,GPoint(16,181),tone);
  text(ctx,delay,29,169,157,24,FONT_KEY_GOTHIC_18_BOLD,tone,GTextAlignmentLeft);
  fill(ctx,GRect(10,193,177,1),GColorMayGreen);
  label(ctx,"TERMINAL",10,194,89);
  label(ctx,departure?"GATE":"BAGGAGE",106,194,82);
  text(ctx,flight_field_text(airport->terminal),10,207,88,21,FONT_KEY_GOTHIC_18_BOLD,GColorWhite,GTextAlignmentLeft);
  text(ctx,flight_field_text(airport->facility),106,207,82,21,FONT_KEY_GOTHIC_18_BOLD,GColorWhite,GTextAlignmentLeft);
}
static void draw_booking(GContext *ctx, const Flight *flight) {
  const FlightBooking *booking=&flight->booking;
  char heading[40];
  snprintf(heading,sizeof(heading),"%s / BOOKING",flight->number);
  label(ctx,heading,10,1,178);

  booking_code_icon_draw(ctx,GPoint(14,29));
  label(ctx,"BOOKING CODE",54,33,132);
  const char *code=flight_field_text(booking->code);
  const char *code_fonts[]={FONT_KEY_BITHAM_30_BLACK,FONT_KEY_GOTHIC_24_BOLD,FONT_KEY_GOTHIC_18_BOLD};
  int code_font_index=booking->code.state==FIELD_KNOWN?0:1;
  // Fit the whole booking reference, including wide 12-character values.
  while (code_font_index<2) {
    GSize size=graphics_text_layout_get_content_size(code,font(code_fonts[code_font_index]),
      GRect(0,0,1000,100),GTextOverflowModeFill,GTextAlignmentLeft);
    if (size.w<=177) { break; }
    code_font_index++;
  }
  const char *code_font=code_fonts[code_font_index];
  text(ctx,code,10,58,177,38,code_font,GColorWhite,GTextAlignmentLeft);
  fill(ctx,GRect(10,101,177,1),GColorMayGreen);

  booking_seat_icon_draw(ctx,GPoint(27,104),booking_seat_position(booking));
  label(ctx,"SEAT",134,112,54);
  text(ctx,flight_field_text(booking->seat),132,131,56,39,
       booking->seat.state==FIELD_KNOWN?FONT_KEY_BITHAM_30_BLACK:
       booking->seat.state==FIELD_UNKNOWN?FONT_KEY_GOTHIC_14_BOLD:FONT_KEY_GOTHIC_18_BOLD,
       accent(),GTextAlignmentLeft);
  text(ctx,booking_position_label(booking),10,181,177,25,FONT_KEY_GOTHIC_18_BOLD,
       GColorWhite,GTextAlignmentLeft);
  text(ctx,
#if PEBBLE_MOCK_DATA
       "MOCK / sample booking",
#else
       "Private / saved on phone",
#endif
       10,210,177,18,FONT_KEY_GOTHIC_14,
       accent(),GTextAlignmentLeft);
}
static void draw_note(GContext *ctx, const Flight *flight) {
  label(ctx,"ABOUT THIS FLIGHT",10,3,178);
  text(ctx,flight->number,10,23,176,32,FONT_KEY_GOTHIC_28_BOLD,GColorWhite,GTextAlignmentLeft);
#if PEBBLE_MOCK_DATA
  text(ctx,"MOCK / fixed example\n24h airport-local times\nUTC offsets include DST\nUnknown = not reported\nN/A = unavailable",10,64,177,112,FONT_KEY_GOTHIC_18,GColorWhite,GTextAlignmentLeft);
#else
  const NativeFlight *m=live_metadata_at(s_nav.selected);
  char information[512];
  const char *error=m && m->text[TX_ERROR][0]?m->text[TX_ERROR]:live_global_error();
  snprintf(information,sizeof(information),"%s / 24h airport-local\nUpdated %s\nNext auto %s\n%s\nUnknown = not reported\nN/A = unavailable",source_label(),
    m?m->text[TX_UPDATED]:"Never",m?m->text[TX_NEXT_REFRESH]:"--",
    error[0]?error:"Times include local dates and DST.");
  text(ctx,information,10,60,177,120,FONT_KEY_GOTHIC_14,GColorWhite,GTextAlignmentLeft);
#endif
  text(ctx,"SELECT: close note\nBACK: all flights",10,184,177,41,FONT_KEY_GOTHIC_14_BOLD,accent(),GTextAlignmentLeft);
}
#if !PEBBLE_MOCK_DATA
static void draw_aircraft(GContext *ctx,const Flight *flight) {
  const NativeFlight *m=live_metadata_at(s_nav.selected);
  if (!m) { return; }
  label(ctx,"AIRCRAFT",10,1,178);
  text(ctx,m->text[TX_AIRCRAFT_MODEL],10,24,177,53,FONT_KEY_GOTHIC_24_BOLD,GColorWhite,GTextAlignmentLeft);
  text(ctx,m->text[TX_AIRCRAFT_NUMBER],10,76,177,28,FONT_KEY_GOTHIC_24_BOLD,accent(),GTextAlignmentLeft);
  label(ctx,"FIRST FLIGHT",10,108,90);
  text(ctx,m->text[TX_AIRCRAFT_FIRST],101,108,87,18,FONT_KEY_GOTHIC_14,GColorWhite,GTextAlignmentRight);
  label(ctx,"INCOMING LEGS",10,133,178);
  text(ctx,m->text[TX_LEG2],10,150,92,18,FONT_KEY_GOTHIC_14,GColorWhite,GTextAlignmentLeft);
  text(ctx,m->text[TX_LEG2_STATUS],104,150,84,18,FONT_KEY_GOTHIC_14,accent(),GTextAlignmentRight);
  text(ctx,m->text[TX_LEG1],10,174,92,18,FONT_KEY_GOTHIC_14,GColorWhite,GTextAlignmentLeft);
  text(ctx,m->text[TX_LEG1_STATUS],104,174,84,18,FONT_KEY_GOTHIC_14,accent(),GTextAlignmentRight);
  text(ctx,"BACK: flight details",10,210,177,18,FONT_KEY_GOTHIC_14,GColorWhite,GTextAlignmentLeft);
}
#endif
static void draw_scene(Layer *layer, GContext *ctx) {
  fill(ctx,layer_get_bounds(layer),background());
  if (s_nav.page==PAGE_LIST) { draw_list(ctx); return; }
  const Flight *flight=data_at(s_nav.selected);
  if (!flight) { return; }
#if !PEBBLE_MOCK_DATA
  if (s_aircraft) { draw_aircraft(ctx,flight); return; }
#endif
  if (s_nav.note) { draw_note(ctx,flight); }
  else if (s_nav.page==PAGE_OVERVIEW) { draw_overview(ctx,flight); }
  else if (s_nav.page==PAGE_BOOKING) { draw_booking(ctx,flight); }
  else { draw_airport(ctx,flight,s_nav.page==PAGE_DEPARTURE); }
}
static void canvas_update(Layer *layer, GContext *ctx) { draw_scene(layer,ctx); draw_emphasis(ctx); }
static void outgoing_update(Layer *layer, GContext *ctx) {
  PrototypeNavigation current = s_nav;
  s_nav = s_before;
#if !PEBBLE_MOCK_DATA
  bool aircraft = s_aircraft;
  s_aircraft = s_before_aircraft;
#endif
  s_drawing_old = true;
  draw_scene(layer,ctx);
  s_drawing_old = false;
  s_nav = current;
#if !PEBBLE_MOCK_DATA
  s_aircraft = aircraft;
#endif
}
static void indicators_update(Layer *layer, GContext *ctx) {
  if (s_nav.page != PAGE_LIST && !aircraft_showing()) {
    fill(ctx,GRect(190,0,10,228),background());
    page_indicator(ctx);
  }
  if (s_activity_timer) {
    graphics_context_set_fill_color(ctx,accent());
    for (int i=0; i<3; ++i) {
      graphics_fill_circle(ctx,GPoint(194,149+i*5),i==(int)(s_activity_frame%3)?2:1);
    }
  }
}
static void hero_update(Layer *layer, GContext *ctx) {
  bool opening = s_move == MOVE_OPEN;
  if (!opening && !(s_move == MOVE_CLOSE && s_before.page == PAGE_OVERVIEW && !s_before.note)) { return; }
  const Flight *flight = data_at(s_nav.selected);
  if (!flight) { return; }
  int p = motion_ease(opening?s_move_p:MOTION_UNIT-s_move_p);
  int row_y = 49 + ((int)s_nav.selected - (int)s_nav.top)*51;
  GSize size = graphics_text_layout_get_content_size(flight->number,font(FONT_KEY_GOTHIC_18_BOLD),
    GRect(0,0,190,25),GTextOverflowModeTrailingEllipsis,GTextAlignmentLeft);
  text(ctx,flight->number,motion_mix(10,98-size.w/2,p),motion_mix(row_y,91,p),
       150,25,FONT_KEY_GOTHIC_18_BOLD,GColorWhite,GTextAlignmentLeft);
}
static void top_layer(Layer *layer) {
  layer_remove_from_parent(layer);
  layer_add_child(window_get_root_layer(s_window),layer);
}
static void clip_scene(Layer *clip, Layer *canvas, GRect rect, int dx, int dy) {
  layer_set_frame(clip,rect);
  layer_set_frame(canvas,GRect(dx-rect.origin.x,dy-rect.origin.y,200,228));
}
static void motion_layout(void) {
  GRect full = GRect(0,0,200,228);
  clip_scene(s_in_clip,s_canvas,full,0,0);
  clip_scene(s_out_clip,s_outgoing,full,0,0);
  if (s_move == MOVE_PAGE) {
    int direction = s_nav.page > s_before.page ? 1 : -1;
    int travel = motion_travel(228,3,s_move_p);
    clip_scene(s_out_clip,s_outgoing,full,0,-direction*(travel>228?228:travel));
    clip_scene(s_in_clip,s_canvas,full,0,direction*(228-travel));
  } else if (s_move == MOVE_OPEN || s_move == MOVE_CLOSE) {
    int p = motion_ease(s_move == MOVE_OPEN?s_move_p:MOTION_UNIT-s_move_p);
    int row_y = 51+((int)s_nav.selected-(int)s_nav.top)*51;
    GRect rect = GRect(0,motion_mix(row_y,0,p),motion_mix(190,200,p),motion_mix(51,228,p));
    clip_scene(s_move == MOVE_OPEN?s_in_clip:s_out_clip,
               s_move == MOVE_OPEN?s_canvas:s_outgoing,rect,0,0);
  } else if (s_move == MOVE_NOTE) {
    bool closing = s_before.note && !s_nav.note;
    // A quick wipe plus a shallow lift keeps supplemental information distinct.
    int p = motion_ease(closing?MOTION_UNIT-s_move_p:s_move_p);
    GRect rect = GRect(0,0,motion_mix(1,200,p),228);
    clip_scene(closing?s_out_clip:s_in_clip,closing?s_outgoing:s_canvas,
               rect,0,motion_mix(8,0,p));
  }
  layer_mark_dirty(s_canvas);
  layer_mark_dirty(s_indicators);
  layer_mark_dirty(s_hero);
}
static void motion_stopped(Animation *animation, bool finished, void *context) {
  s_move_anim = NULL;
  s_move = MOVE_NONE;
  s_move_p = MOTION_UNIT;
  if (s_canvas) {
    motion_layout();
    layer_set_hidden(s_out_clip,true);
  }
  animation_destroy(animation);
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Motion settled%s",finished?"":" / interrupted");
}
static void motion_update(Animation *animation, AnimationProgress progress) {
  s_move_p = (int)((int64_t)progress*MOTION_UNIT/ANIMATION_NORMALIZED_MAX);
  motion_layout();
}
static const AnimationImplementation s_move_impl = { .update = motion_update };
static void motion_stop(void) {
  if (s_move_anim) { animation_unschedule(s_move_anim); }
}
static void motion_start(PrototypeNavigation before, bool before_aircraft) {
  motion_stop();
  s_before = before;
  s_before_aircraft = before_aircraft;
  if (before.page == PAGE_LIST && s_nav.page != PAGE_LIST) { s_move = MOVE_OPEN; }
  else if (before.page != PAGE_LIST && s_nav.page == PAGE_LIST) { s_move = MOVE_CLOSE; }
  else if (before.page != s_nav.page) { s_move = MOVE_PAGE; }
  else if (before.note != s_nav.note || before.empty != s_nav.empty || before_aircraft != aircraft_showing()) { s_move = MOVE_NOTE; }
  else if (before.selected != s_nav.selected || before.top != s_nav.top) { s_move = MOVE_SELECTION; }
  else { return; }
  unsigned duration = s_move == MOVE_SELECTION ? MOTION_SELECTION_MS : s_move == MOVE_PAGE ? MOTION_PAGE_MS :
                      s_move == MOVE_NOTE ? MOTION_NOTE_MS : MOTION_FLIGHT_MS;
  s_move_p = 0;
  top_layer(s_out_clip);
  if (s_move != MOVE_CLOSE && !(s_move == MOVE_NOTE && before.note && !s_nav.note)) { top_layer(s_in_clip); }
  top_layer(s_indicators);
  top_layer(s_hero);
  layer_set_hidden(s_out_clip,s_move == MOVE_SELECTION);
  layer_mark_dirty(s_outgoing);
  s_move_anim = animation_create();
  if (!s_move_anim) { s_move = MOVE_NONE; s_move_p = MOTION_UNIT; layer_set_hidden(s_out_clip,true); motion_layout(); return; }
  animation_set_duration(s_move_anim,duration);
  animation_set_curve(s_move_anim,AnimationCurveLinear);
  animation_set_implementation(s_move_anim,&s_move_impl);
  animation_set_handlers(s_move_anim,(AnimationHandlers){.stopped=motion_stopped},NULL);
  motion_layout();
  animation_schedule(s_move_anim);
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Motion %d / %ums",s_move,duration);
}

typedef struct {
  char key[100];
  uint32_t fields[9];
  int progress;
  bool valid;
} SeenFlight;
static SeenFlight s_seen;
static uint32_t hash_text(const char *value) {
  uint32_t result = 2166136261U;
  for (const unsigned char *p=(const unsigned char *)(value?value:""); *p; ++p) { result = (result^*p)*16777619U; }
  return result;
}
static SeenFlight visible_flight(void) {
  SeenFlight seen = {0};
  const Flight *flight = s_nav.empty?NULL:data_at(s_nav.selected);
  if (!flight) { return seen; }
  snprintf(seen.key,sizeof(seen.key),"%s/%s",flight->number,flight->date);
#if !PEBBLE_MOCK_DATA
  const NativeFlight *metadata = live_metadata_at(s_nav.selected);
  if (metadata) { snprintf(seen.key,sizeof(seen.key),"%s",metadata->text[TX_KEY]); }
#endif
  seen.progress = flight_progress(flight);
  seen.fields[0] = flight->status;
  seen.fields[1] = (hash_text(flight->departure.time.current)*16777619U)^hash_text(flight->departure.time.scheduled)^flight->departure.time.kind;
  seen.fields[2] = (hash_text(flight->arrival.time.current)*16777619U)^hash_text(flight->arrival.time.scheduled)^flight->arrival.time.kind;
  seen.fields[3] = hash_text(flight_field_text(flight->departure.terminal));
  seen.fields[4] = hash_text(flight_field_text(flight->departure.facility));
  seen.fields[5] = hash_text(flight_field_text(flight->arrival.terminal));
  seen.fields[6] = hash_text(flight_field_text(flight->arrival.facility));
  seen.fields[7] = hash_text(flight_field_text(flight->booking.code));
  seen.fields[8] = hash_text(flight_field_text(flight->booking.seat))^booking_seat_position(&flight->booking);
  seen.valid = true;
  return seen;
}
static void remember_flight(void) { s_seen = visible_flight(); }
#if !PEBBLE_MOCK_DATA || PEBBLE_MOTION_DEMO
static void progress_update(Animation *animation, AnimationProgress progress) {
  s_progress_p = (int)((int64_t)progress*MOTION_UNIT/ANIMATION_NORMALIZED_MAX);
  layer_mark_dirty(s_canvas);
}
static void progress_stopped(Animation *animation, bool finished, void *context) {
  s_progress_anim = NULL;
  s_progress_p = MOTION_UNIT;
  animation_destroy(animation);
  if (s_canvas) { layer_mark_dirty(s_canvas); }
}
static const AnimationImplementation s_progress_impl = { .update = progress_update };
static void emphasis_end(void *context) {
  s_emphasis_timer = NULL;
  s_emphasis = 0;
  if (s_canvas) { layer_mark_dirty(s_canvas); }
}
#endif
static void feedback_stop(void) {
  if (s_progress_anim) { animation_unschedule(s_progress_anim); }
  if (s_emphasis_timer) { app_timer_cancel(s_emphasis_timer); s_emphasis_timer = NULL; }
  s_emphasis = 0;
}
static void activity_step(void *context) {
  s_activity_timer = NULL;
  ++s_activity_frame;
  activity_sync();
}
static void activity_sync(void) {
  bool loading = false;
#if PEBBLE_MOCK_DATA && PEBBLE_MOTION_DEMO
  loading = s_demo_loading && s_demo_target == s_nav.selected;
#endif
#if !PEBBLE_MOCK_DATA
  const NativeFlight *metadata = live_metadata_at(s_nav.selected);
  loading = metadata && metadata->loading && !metadata->text[TX_ERROR][0] &&
            !live_global_error()[0] && connection_service_peek_pebble_app_connection();
#endif
  if (s_visible && loading) {
    if (!s_activity_timer) { s_activity_timer = app_timer_register(120,activity_step,NULL); }
  } else if (s_activity_timer) { app_timer_cancel(s_activity_timer); s_activity_timer = NULL; }
  if (s_indicators) { layer_mark_dirty(s_indicators); }
}
static void emphasize(GContext *ctx, unsigned field, int x, int y, int h) {
  if (!s_drawing_old && (s_emphasis & field)) { fill(ctx,GRect(x,y,2,h),accent()); }
}
static void draw_emphasis(GContext *ctx) {
  if (s_nav.note || s_nav.page == PAGE_LIST || aircraft_showing()) { return; }
  if (s_nav.page == PAGE_OVERVIEW) {
    emphasize(ctx,CH_STATUS,7,132,12);
    emphasize(ctx,CH_DEP_TIME,7,198,12); emphasize(ctx,CH_ARR_TIME,103,198,12);
  } else if (s_nav.page == PAGE_DEPARTURE || s_nav.page == PAGE_ARRIVAL) {
    bool dep = s_nav.page == PAGE_DEPARTURE;
    emphasize(ctx,dep?CH_DEP_TIME:CH_ARR_TIME,103,132,14);
    emphasize(ctx,dep?CH_DEP_TERMINAL:CH_ARR_TERMINAL,7,212,12);
    emphasize(ctx,dep?CH_GATE:CH_BAGGAGE,103,212,12);
  } else {
    emphasize(ctx,CH_BOOKING,7,69,20); emphasize(ctx,CH_SEAT,129,142,20);
  }
}
#if !PEBBLE_MOCK_DATA || PEBBLE_MOTION_DEMO
static void feedback_changed(SeenFlight before, int prior_progress) {
  if (!s_visible || s_nav.page == PAGE_LIST || !before.valid || !s_seen.valid || strcmp(before.key,s_seen.key)) { return; }
  for (int i=0; i<9; ++i) { if (before.fields[i] != s_seen.fields[i]) { s_emphasis |= 1U<<i; } }
  if (s_emphasis) { s_emphasis_timer = app_timer_register(MOTION_EMPHASIS_MS,emphasis_end,NULL); }
  if (s_nav.page == PAGE_OVERVIEW && !s_nav.note && !aircraft_showing() &&
      before.progress >= 0 && s_seen.progress >= 0 && prior_progress != s_seen.progress) {
    s_progress_from = prior_progress; s_progress_to = s_seen.progress; s_progress_p = 0;
    s_progress_anim = animation_create();
    if (s_progress_anim) {
      animation_set_duration(s_progress_anim,MOTION_PROGRESS_MS);
      animation_set_curve(s_progress_anim,AnimationCurveLinear);
      animation_set_implementation(s_progress_anim,&s_progress_impl);
      animation_set_handlers(s_progress_anim,(AnimationHandlers){.stopped=progress_stopped},NULL);
      animation_schedule(s_progress_anim);
    }
  }
}
#endif
#if PEBBLE_MOCK_DATA && PEBBLE_MOTION_DEMO
static void demo_refresh_done(void *context) {
  s_demo_timer = NULL;
  SeenFlight before = s_seen;
  feedback_stop(); motion_stop();
  const Flight *flight = data_at(s_demo_target);
  if (flight) {
    s_demo_flight = *flight;
    s_demo_index = s_demo_target; s_demo_valid = true;
    if (s_demo_flight.status == FLIGHT_IN_FLIGHT) {
      s_demo_flight.progress_percent = s_demo_flight.progress_percent == 62 ? 70 : 62;
    }
    if (s_demo_flight.status != FLIGHT_CANCELED && s_demo_flight.status != FLIGHT_DIVERTED) {
      const char *gate = flight_field_text(s_demo_flight.departure.facility);
      s_demo_flight.departure.facility = (FlightField){FIELD_KNOWN,!strcmp(gate,"B12")?"A6":"B12"};
    }
  }
  s_demo_loading = false;
  remember_flight(); activity_sync();
  feedback_changed(before,before.progress);
  layer_mark_dirty(s_canvas);
}
static void demo_refresh(void) {
  if (s_demo_timer) { app_timer_cancel(s_demo_timer); }
  s_demo_target = s_nav.selected; s_demo_loading = true;
  s_demo_timer = app_timer_register(1000,demo_refresh_done,NULL);
  activity_sync(); layer_mark_dirty(s_canvas);
}
#endif
static void changed(void) {
  APP_LOG(APP_LOG_LEVEL_INFO,"UI page=%d selected=%d top=%d empty=%d note=%d",
          s_nav.page,(int)s_nav.selected,(int)s_nav.top,s_nav.empty,s_nav.note);
#if !PEBBLE_MOCK_DATA
  const NativeFlight *metadata=live_metadata_at(s_nav.selected);
  if (metadata) { snprintf(s_selected_key,sizeof(s_selected_key),"%s",metadata->text[TX_KEY]); }
#endif
  if (s_canvas) { layer_mark_dirty(s_canvas); }
  remember_flight();
  activity_sync();
}
static PrototypeNavigation before_navigation(void) {
  motion_stop();
  feedback_stop();
  return s_nav;
}
static void up(ClickRecognizerRef r, void *c) {
#if !PEBBLE_MOCK_DATA
  if (s_aircraft) { return; }
#endif
  PrototypeNavigation before = before_navigation();
  navigation_move(&s_nav,-1,data_count()); motion_start(before,false); changed(); }
static void down(ClickRecognizerRef r, void *c) {
#if !PEBBLE_MOCK_DATA
  if (s_aircraft) { return; }
#endif
  PrototypeNavigation before = before_navigation();
  navigation_move(&s_nav,1,data_count()); motion_start(before,false); changed(); }
static void select_click(ClickRecognizerRef r, void *c) {
  PrototypeNavigation before = before_navigation();
  bool aircraft = aircraft_showing();
#if !PEBBLE_MOCK_DATA
  if (s_aircraft) { s_aircraft=false; motion_start(before,aircraft); changed(); return; }
  bool opening=s_nav.page==PAGE_LIST && data_count();
#endif
  navigation_select(&s_nav,data_count()); motion_start(before,aircraft); changed();
#if !PEBBLE_MOCK_DATA
  if (opening) { live_select_flight(s_nav.selected,false); }
#endif
}
static void long_select(ClickRecognizerRef r, void *c) {
#if PEBBLE_MOCK_DATA
  PrototypeNavigation before = before_navigation();
  if (s_nav.page == PAGE_LIST) { navigation_toggle_empty(&s_nav); motion_start(before,false); changed(); }
#if PEBBLE_MOTION_DEMO
  else { demo_refresh(); }
#endif
#else
  before_navigation();
  if (data_count()) { window_stack_push(s_actions_window,true); }
#endif
}
static void back(ClickRecognizerRef r, void *c) {
  PrototypeNavigation before = before_navigation();
  bool aircraft = aircraft_showing();
#if !PEBBLE_MOCK_DATA
  if (s_aircraft) { s_aircraft=false; motion_start(before,aircraft); changed(); return; }
#endif
  if (navigation_back(&s_nav)) { window_stack_pop(true); }
  else { motion_start(before,aircraft); changed(); }
}
#if !PEBBLE_MOCK_DATA
static uint16_t action_rows(MenuLayer *menu,uint16_t section,void *context) { return 3; }
static void action_draw(GContext *ctx,const Layer *layer,MenuIndex *index,void *context) {
  const char *names[]={"Refresh flight","Aircraft details","Close"};
  menu_cell_basic_draw(ctx,layer,names[index->row],NULL,NULL);
}
static void action_select(MenuLayer *menu,MenuIndex *index,void *context) {
  int action=index->row;
  window_stack_pop(true);
  if (!data_count()) { return; }
  if (action==0) { live_select_flight(s_nav.selected,true); }
  else if (action==1) {
    PrototypeNavigation before = before_navigation();
    if (s_nav.page==PAGE_LIST) { navigation_select(&s_nav,data_count()); }
    s_aircraft=true; s_nav.note=false; motion_start(before,false); live_aircraft(s_nav.selected); changed();
  }
}
static void actions_load(Window *window) {
  Layer *root=window_get_root_layer(window);
  s_actions_menu=menu_layer_create(layer_get_bounds(root));
  menu_layer_set_normal_colors(s_actions_menu,background(),GColorWhite);
  menu_layer_set_highlight_colors(s_actions_menu,accent(),background());
  menu_layer_set_callbacks(s_actions_menu,NULL,(MenuLayerCallbacks){
    .get_num_rows=action_rows,.draw_row=action_draw,.select_click=action_select});
  menu_layer_set_click_config_onto_window(s_actions_menu,window);
  layer_add_child(root,menu_layer_get_layer(s_actions_menu));
}
static void actions_unload(Window *window) { menu_layer_destroy(s_actions_menu); s_actions_menu=NULL; }
static void data_changed(void) {
  SeenFlight before = s_seen;
  int prior_progress = s_progress_anim ? motion_mix(s_progress_from,s_progress_to,motion_ease(s_progress_p)) : before.progress;
  motion_stop();
  feedback_stop();
  size_t count=data_count();
  s_nav.empty=count==0;
  bool found=false;
  for (size_t i=0;i<count;i++) {
    if (!strcmp(live_metadata_at(i)->text[TX_KEY],s_selected_key)) { s_nav.selected=i; found=true; break; }
  }
  if (!found && s_selected_key[0]) { s_nav.page=PAGE_LIST; s_nav.note=false; s_aircraft=false; }
  if (!count) { s_nav.selected=0; s_nav.top=0; s_nav.page=PAGE_LIST; s_nav.note=false; s_aircraft=false; }
  else {
    if (s_nav.selected>=count) { s_nav.selected=count-1; }
    size_t max_top=count>VISIBLE_FLIGHTS?count-VISIBLE_FLIGHTS:0;
    if (s_nav.top>max_top) { s_nav.top=max_top; }
    if (s_nav.selected<s_nav.top) { s_nav.top=s_nav.selected; }
  }
  changed();
  const NativeFlight *metadata = live_metadata_at(s_nav.selected);
  if (metadata && !metadata->loading && !metadata->stale &&
      !metadata->text[TX_ERROR][0] && !live_global_error()[0]) {
    feedback_changed(before,prior_progress);
  }
}
#endif
static void click_config(void *context) {
  window_single_repeating_click_subscribe(BUTTON_ID_UP,180,up);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN,180,down);
  window_single_click_subscribe(BUTTON_ID_SELECT,select_click);
  window_long_click_subscribe(BUTTON_ID_SELECT,700,long_select,NULL);
  window_single_click_subscribe(BUTTON_ID_BACK,back);
}
static void window_load(Window *window) {
  Layer *root=window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);
  s_out_clip=layer_create(bounds);
  s_outgoing=layer_create(bounds);
  layer_set_update_proc(s_outgoing,outgoing_update);
  layer_add_child(s_out_clip,s_outgoing);
  layer_add_child(root,s_out_clip);
  layer_set_hidden(s_out_clip,true);
  s_in_clip=layer_create(bounds);
  s_canvas=layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_canvas,canvas_update);
  layer_add_child(s_in_clip,s_canvas);
  layer_add_child(root,s_in_clip);
  s_indicators=layer_create(bounds);
  layer_set_update_proc(s_indicators,indicators_update);
  layer_add_child(root,s_indicators);
  s_hero=layer_create(bounds);
  layer_set_update_proc(s_hero,hero_update);
  layer_add_child(root,s_hero);
}
static void window_appear(Window *window) { s_visible = true; activity_sync(); }
static void window_disappear(Window *window) { s_visible = false; motion_stop(); feedback_stop(); activity_sync(); }
static void window_unload(Window *window) {
  motion_stop(); feedback_stop();
#if PEBBLE_MOCK_DATA && PEBBLE_MOTION_DEMO
  if (s_demo_timer) { app_timer_cancel(s_demo_timer); s_demo_timer = NULL; }
#endif
  if (s_activity_timer) { app_timer_cancel(s_activity_timer); s_activity_timer = NULL; }
  layer_destroy(s_hero); layer_destroy(s_indicators);
  layer_destroy(s_canvas); layer_destroy(s_in_clip);
  layer_destroy(s_outgoing); layer_destroy(s_out_clip);
  s_canvas=NULL; s_indicators=NULL;
}
int main(void) {
  pebble_plane_init();
  booking_icons_init();
  s_window=window_create();
  window_set_background_color(s_window,background());
  window_set_click_config_provider(s_window,click_config);
  window_set_window_handlers(s_window,(WindowHandlers){.load=window_load,.unload=window_unload,
    .appear=window_appear,.disappear=window_disappear});
  window_stack_push(s_window,true);
#if !PEBBLE_MOCK_DATA
  s_actions_window=window_create();
  window_set_window_handlers(s_actions_window,(WindowHandlers){.load=actions_load,.unload=actions_unload});
  s_nav.empty=true;
  live_data_init(data_changed);
#endif
  app_event_loop();
#if !PEBBLE_MOCK_DATA
  live_data_deinit();
  window_destroy(s_actions_window);
#endif
  window_destroy(s_window);
  pebble_plane_deinit();
  booking_icons_deinit();
  airline_icons_deinit();
}
#endif
