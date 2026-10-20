#include "../prototype_config.h"
#if PEBBLE_UI_PROTOTYPE
#include <pebble.h>
#include "flight_model.h"
#include "navigation.h"
#include "pebble_plane.h"
#include "booking_icons.h"
#include "airline_icons.h"
#include "live_data.h"

static Window *s_window;
static Layer *s_canvas;
static PrototypeNavigation s_nav = { .page = PAGE_LIST };
#if !PEBBLE_MOCK_DATA
static Window *s_actions_window;
static MenuLayer *s_actions_menu;
static bool s_aircraft;
static char s_selected_key[LIVE_TEXT_SIZE];
#endif
static size_t data_count(void) {
#if PEBBLE_MOCK_DATA
  return mock_flight_count();
#else
  return live_flight_count();
#endif
}
static const Flight *data_at(size_t index) {
#if PEBBLE_MOCK_DATA
  return mock_flight_at(index);
#else
  return live_flight_at(index);
#endif
}
static const char *source_label(void) {
#if PEBBLE_MOCK_DATA
  return "MOCK";
#else
  const NativeFlight *m=live_metadata_at(s_nav.selected);
  if (!connection_service_peek_pebble_app_connection()) { return "OFFLINE"; }
  if (live_global_error()[0]) { return "SAVED"; }
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
  for (int i=0; i<4; i++) {
    graphics_context_set_fill_color(ctx, i==s_nav.page ? GColorWhite : GColorMayGreen);
    graphics_fill_circle(ctx, GPoint(194,88+i*10), i==s_nav.page ? 3 : 2);
  }
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
    pebble_plane_draw(ctx,GPoint(98,84));
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
  for (size_t row=0; row<VISIBLE_FLIGHTS; row++) {
    size_t index=s_nav.top+row;
    const Flight *flight=data_at(index);
    if (!flight) { break; }
    int y=51+(int)row*51;
    bool selected=index==s_nav.selected;
    if (selected) {
      fill(ctx,GRect(0,y,190,51),background());
      fill(ctx,GRect(0,y,3,51),accent());
    }
    text(ctx,flight->number,10,y-2,100,24,FONT_KEY_GOTHIC_18_BOLD,GColorWhite,GTextAlignmentLeft);
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
      graphics_draw_line(ctx,GPoint(179,y+22),GPoint(183,y+26));
      graphics_draw_line(ctx,GPoint(183,y+26),GPoint(179,y+30));
    }
  }
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
  graphics_context_set_stroke_width(ctx,8);
  graphics_context_set_stroke_color(ctx,GColorMayGreen);
  graphics_draw_arc(ctx,track,GOvalScaleModeFitCircle,start,start+sweep);
  if (progress>0) {
    graphics_context_set_stroke_color(ctx,accent());
    graphics_draw_arc(ctx,track,GOvalScaleModeFitCircle,start,start+sweep*progress/100);
  }
  if (progress>=0) {
    GPoint marker=gpoint_from_polar(track,GOvalScaleModeFitCircle,start+sweep*progress/100);
    pebble_plane_progress_draw(ctx,marker,flight->status == FLIGHT_IN_FLIGHT);
    char percent[16];
    snprintf(percent,sizeof(percent),"%d%%",progress);
    text(ctx,percent,56,40,84,28,FONT_KEY_GOTHIC_24_BOLD,
         GColorWhite,GTextAlignmentCenter);
  } else {
    if (progress==-1) { status_icon(ctx,FLIGHT_CANCELED,GPoint(98,39),GColorWhite); }
    text(ctx,progress==-1?"NO JOURNEY":"PROGRESS",56,progress==-1?44:29,84,18,FONT_KEY_GOTHIC_14,GColorWhite,GTextAlignmentCenter);
    if (progress==-2) { text(ctx,"UNKNOWN",56,43,84,18,FONT_KEY_GOTHIC_14,GColorWhite,GTextAlignmentCenter); }
  }
  if (!airline_icon_draw(ctx,flight->number,GPoint(86,66))) {
    pebble_plane_draw(ctx,GPoint(98,78));
  }
  text(ctx,flight->number,51,91,94,25,FONT_KEY_GOTHIC_18_BOLD,GColorWhite,GTextAlignmentCenter);
  status_icon(ctx,flight->status,GPoint(23,136),status_color(flight));
  char status[40];
  text(ctx,status_text(flight,status,sizeof(status)),35,123,145,25,FONT_KEY_GOTHIC_18_BOLD,status_color(flight),GTextAlignmentCenter);
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
static void canvas_update(Layer *layer, GContext *ctx) {
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
  page_indicator(ctx);
}
static void changed(void) {
  APP_LOG(APP_LOG_LEVEL_INFO,"UI page=%d selected=%d top=%d empty=%d note=%d",
          s_nav.page,(int)s_nav.selected,(int)s_nav.top,s_nav.empty,s_nav.note);
#if !PEBBLE_MOCK_DATA
  const NativeFlight *metadata=live_metadata_at(s_nav.selected);
  if (metadata) { snprintf(s_selected_key,sizeof(s_selected_key),"%s",metadata->text[TX_KEY]); }
#endif
  if (s_canvas) { layer_mark_dirty(s_canvas); }
}
static void up(ClickRecognizerRef r, void *c) {
#if !PEBBLE_MOCK_DATA
  if (s_aircraft) { return; }
#endif
  navigation_move(&s_nav,-1,data_count()); changed(); }
static void down(ClickRecognizerRef r, void *c) {
#if !PEBBLE_MOCK_DATA
  if (s_aircraft) { return; }
#endif
  navigation_move(&s_nav,1,data_count()); changed(); }
static void select_click(ClickRecognizerRef r, void *c) {
#if !PEBBLE_MOCK_DATA
  if (s_aircraft) { s_aircraft=false; changed(); return; }
  bool opening=s_nav.page==PAGE_LIST && data_count();
#endif
  navigation_select(&s_nav,data_count()); changed();
#if !PEBBLE_MOCK_DATA
  if (opening) { live_select_flight(s_nav.selected,false); }
#endif
}
static void long_select(ClickRecognizerRef r, void *c) {
#if PEBBLE_MOCK_DATA
  navigation_toggle_empty(&s_nav); changed();
#else
  if (data_count()) { window_stack_push(s_actions_window,true); }
#endif
}
static void back(ClickRecognizerRef r, void *c) {
#if !PEBBLE_MOCK_DATA
  if (s_aircraft) { s_aircraft=false; changed(); return; }
#endif
  if (navigation_back(&s_nav)) { window_stack_pop(true); }
  else { changed(); }
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
    if (s_nav.page==PAGE_LIST) { navigation_select(&s_nav,data_count()); }
    s_aircraft=true; s_nav.note=false; live_aircraft(s_nav.selected); changed();
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
  s_canvas=layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_canvas,canvas_update);
  layer_add_child(root,s_canvas);
}
static void window_unload(Window *window) { layer_destroy(s_canvas); s_canvas=NULL; }
int main(void) {
  pebble_plane_init();
  booking_icons_init();
  s_window=window_create();
  window_set_background_color(s_window,background());
  window_set_click_config_provider(s_window,click_config);
  window_set_window_handlers(s_window,(WindowHandlers){.load=window_load,.unload=window_unload});
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
