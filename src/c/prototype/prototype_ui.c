#include "../prototype_config.h"
#if PEBBLE_UI_PROTOTYPE
#include <pebble.h>
#include "flight_model.h"
#include "navigation.h"
#include "pebble_plane.h"

static Window *s_window;
static Layer *s_canvas;
static PrototypeNavigation s_nav = { .page = PAGE_LIST };

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
  if (flight->status == FLIGHT_CANCELED) { return GColorWhite; }
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
  if (status == FLIGHT_CANCELED) {
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
  for (int i=0; i<3; i++) {
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
  if (s_nav.page < PAGE_ARRIVAL) {
    graphics_draw_line(ctx, GPoint(191,130),GPoint(194,133));
    graphics_draw_line(ctx, GPoint(194,133),GPoint(197,130));
  }
}
static void draw_list(GContext *ctx) {
  fill(ctx, GRect(0,0,200,228), GColorBlack);
  text(ctx, "All flights",10,0,148,32,FONT_KEY_GOTHIC_28_BOLD,GColorWhite,GTextAlignmentLeft);
  char count[16];
  snprintf(count,sizeof(count),"%d / %d",s_nav.empty?0:(int)s_nav.selected+1,
           s_nav.empty?0:(int)mock_flight_count());
  text(ctx,count,151,10,39,18,FONT_KEY_GOTHIC_14,GColorWhite,GTextAlignmentRight);
  text(ctx,"MOCK  /  OCT 2026",10,30,178,18,FONT_KEY_GOTHIC_14,accent(),GTextAlignmentLeft);
  if (s_nav.empty) {
    pebble_plane_draw(ctx,GPoint(98,84));
    text(ctx,"No flights yet",10,111,180,32,FONT_KEY_GOTHIC_24_BOLD,GColorWhite,GTextAlignmentCenter);
    text(ctx,"Your logged flights\nwill appear here.",10,147,180,40,FONT_KEY_GOTHIC_18,GColorWhite,GTextAlignmentCenter);
    text(ctx,"Hold SELECT: restore demo",5,209,190,18,FONT_KEY_GOTHIC_14,accent(),GTextAlignmentCenter);
    return;
  }
  for (size_t row=0; row<VISIBLE_FLIGHTS; row++) {
    size_t index=s_nav.top+row;
    const Flight *flight=mock_flight_at(index);
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
  fill(ctx,GRect(195,52,2,150),GColorDarkGray);
  int thumb_h=150*VISIBLE_FLIGHTS/(int)mock_flight_count();
  int thumb_y=52+(150-thumb_h)*(int)s_nav.top/((int)mock_flight_count()-VISIBLE_FLIGHTS);
  fill(ctx,GRect(194,thumb_y,4,thumb_h),GColorWhite);
  text(ctx,"Hold SELECT: empty demo",5,209,190,18,FONT_KEY_GOTHIC_14,GColorLightGray,GTextAlignmentCenter);
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
  text(ctx,"MOCK",145,1,40,18,FONT_KEY_GOTHIC_14,GColorWhite,GTextAlignmentRight);
  GRect ring=GRect(49,29,98,98);
  int start=DEG_TO_TRIGANGLE(225);
  int sweep=DEG_TO_TRIGANGLE(270);
  int progress=flight_progress(flight);
  graphics_context_set_fill_color(ctx,GColorMayGreen);
  graphics_fill_radial(ctx,ring,GOvalScaleModeFitCircle,8,start,start+sweep);
  if (progress>0) {
    graphics_context_set_fill_color(ctx,accent());
    graphics_fill_radial(ctx,ring,GOvalScaleModeFitCircle,8,start,start+sweep*progress/100);
  }
  if (progress>=0) {
    GPoint marker=gpoint_from_polar(ring,GOvalScaleModeFitCircle,start+sweep*progress/100);
    graphics_context_set_fill_color(ctx,background());
    graphics_fill_circle(ctx,marker,14);
    pebble_plane_draw(ctx,marker);
    char percent[16];
    snprintf(percent,sizeof(percent),"%d%%",progress);
    text(ctx,percent,56,49,84,38,progress==100?FONT_KEY_GOTHIC_28_BOLD:FONT_KEY_BITHAM_30_BLACK,
         GColorWhite,GTextAlignmentCenter);
  } else {
    status_icon(ctx,FLIGHT_CANCELED,GPoint(98,68),GColorWhite);
    text(ctx,"NO JOURNEY",56,75,84,18,FONT_KEY_GOTHIC_14,GColorWhite,GTextAlignmentCenter);
  }
  text(ctx,flight->number,51,89,94,25,FONT_KEY_GOTHIC_18_BOLD,GColorWhite,GTextAlignmentCenter);
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
  label(ctx,"SCHEDULED",10,103,94);
  text(ctx,time->scheduled,109,96,78,30,FONT_KEY_GOTHIC_24_BOLD,GColorWhite,GTextAlignmentRight);
  const char *kind=time->kind==TIME_ACTUAL?"ACTUAL":time->kind==TIME_ESTIMATED?"ESTIMATED":
                   time->kind==TIME_UNKNOWN?"NOT REPORTED":"UNAVAILABLE";
  const char *value=time->kind==TIME_UNKNOWN?"Unknown":time->kind==TIME_UNAVAILABLE?"N/A":time->current;
  label(ctx,kind,10,130,94);
  text(ctx,value,106,123,81,30,strlen(value)>5?FONT_KEY_GOTHIC_18_BOLD:FONT_KEY_GOTHIC_24_BOLD,
       accent(),GTextAlignmentRight);
  int offset=time->utc_offset_minutes;
  char zone[48];
  snprintf(zone,sizeof(zone),"%s / %s / UTC%c%d",time->date,time->zone,offset<0?'-':'+',abs(offset)/60);
  if (abs(offset)%60) {
    size_t len=strlen(zone);
    snprintf(zone+len,sizeof(zone)-len,":%02d",abs(offset)%60);
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
static void draw_note(GContext *ctx, const Flight *flight) {
  label(ctx,"ABOUT THIS FLIGHT",10,3,178);
  text(ctx,flight->number,10,23,176,32,FONT_KEY_GOTHIC_28_BOLD,GColorWhite,GTextAlignmentLeft);
  text(ctx,"MOCK / fixed example\n24h airport-local times\nUTC offsets include DST\nUnknown = not reported\nN/A = unavailable",10,64,177,112,FONT_KEY_GOTHIC_18,GColorWhite,GTextAlignmentLeft);
  text(ctx,"SELECT: close note\nBACK: all flights",10,184,177,41,FONT_KEY_GOTHIC_14_BOLD,accent(),GTextAlignmentLeft);
}
static void canvas_update(Layer *layer, GContext *ctx) {
  fill(ctx,layer_get_bounds(layer),background());
  if (s_nav.page==PAGE_LIST) { draw_list(ctx); return; }
  const Flight *flight=mock_flight_at(s_nav.selected);
  if (!flight) { return; }
  if (s_nav.note) { draw_note(ctx,flight); }
  else if (s_nav.page==PAGE_OVERVIEW) { draw_overview(ctx,flight); }
  else { draw_airport(ctx,flight,s_nav.page==PAGE_DEPARTURE); }
  page_indicator(ctx);
}
static void changed(void) {
  APP_LOG(APP_LOG_LEVEL_INFO,"UI page=%d selected=%d top=%d empty=%d note=%d",
          s_nav.page,(int)s_nav.selected,(int)s_nav.top,s_nav.empty,s_nav.note);
  layer_mark_dirty(s_canvas);
}
static void up(ClickRecognizerRef r, void *c) { navigation_move(&s_nav,-1,mock_flight_count()); changed(); }
static void down(ClickRecognizerRef r, void *c) { navigation_move(&s_nav,1,mock_flight_count()); changed(); }
static void select_click(ClickRecognizerRef r, void *c) { navigation_select(&s_nav,mock_flight_count()); changed(); }
static void long_select(ClickRecognizerRef r, void *c) { navigation_toggle_empty(&s_nav); changed(); }
static void back(ClickRecognizerRef r, void *c) {
  if (navigation_back(&s_nav)) { window_stack_pop(true); }
  else { changed(); }
}
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
  s_window=window_create();
  window_set_background_color(s_window,background());
  window_set_click_config_provider(s_window,click_config);
  window_set_window_handlers(s_window,(WindowHandlers){.load=window_load,.unload=window_unload});
  window_stack_push(s_window,true);
  // Deliberately no AppMessage, phone dependency, timers, or persistent writes.
  app_event_loop();
  window_destroy(s_window);
  pebble_plane_deinit();
}
#endif
