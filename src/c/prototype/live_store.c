#include "../prototype_config.h"
#if PEBBLE_UI_PROTOTYPE
#include "live_store.h"
#include <stdio.h>
#include <string.h>
static void copy(char *out,const char *in) { snprintf(out,LIVE_TEXT_SIZE,"%s",in?in:""); }
static void bind(NativeFlight *owned) {
  Flight *f=&owned->flight;
  f->number=owned->text[TX_NUMBER]; f->date=owned->text[TX_DATE];
  f->departure.code=owned->text[TX_ORIGIN]; f->arrival.code=owned->text[TX_DESTINATION];
  f->departure.name=owned->text[TX_DEP_NAME]; f->arrival.name=owned->text[TX_ARR_NAME];
  f->departure.time.scheduled=owned->text[TX_DEP_SCHEDULED];
  f->arrival.time.scheduled=owned->text[TX_ARR_SCHEDULED];
  f->departure.time.current=owned->text[TX_DEP_CURRENT]; f->arrival.time.current=owned->text[TX_ARR_CURRENT];
  f->departure.time.date=owned->text[TX_DEP_DATE]; f->arrival.time.date=owned->text[TX_ARR_DATE];
  f->departure.time.zone=owned->text[TX_DEP_ZONE]; f->arrival.time.zone=owned->text[TX_ARR_ZONE];
  f->departure.time.scheduled_date=owned->text[TX_DEP_SCHEDULED_DATE];
  f->arrival.time.scheduled_date=owned->text[TX_ARR_SCHEDULED_DATE];
  f->departure.time.scheduled_zone=owned->text[TX_DEP_SCHEDULED_ZONE];
  f->arrival.time.scheduled_zone=owned->text[TX_ARR_SCHEDULED_ZONE];
  f->departure.terminal.value=owned->text[TX_DEP_TERMINAL]; f->departure.facility.value=owned->text[TX_GATE];
  f->arrival.terminal.value=owned->text[TX_ARR_TERMINAL]; f->arrival.facility.value=owned->text[TX_BAGGAGE];
  f->booking.code.value=owned->text[TX_BOOKING]; f->booking.seat.value=owned->text[TX_SEAT];
}
static bool valid_field(int state) { return state>=FIELD_KNOWN && state<=FIELD_UNAVAILABLE; }
bool live_store_apply(LiveStore *s,const FlightPacket *p) {
  if (!p->generation || p->generation<s->generation || p->generation<s->pending_generation ||
      p->count<0 || p->count>LIVE_FLIGHT_LIMIT) { return false; }
  if (!p->count) {
    s->count=0; s->generation=p->generation; s->pending_generation=0; s->received=0;
    copy(s->error,p->text[TX_ERROR]); return true;
  }
  if (p->index<0 || p->index>=p->count || !p->text[TX_KEY] || !p->text[TX_KEY][0] ||
      !p->text[TX_NUMBER] || !p->text[TX_NUMBER][0] || !p->text[TX_DATE] ||
      p->status<FLIGHT_UPCOMING || p->status>FLIGHT_UNKNOWN ||
      p->dep_kind<TIME_ESTIMATED || p->dep_kind>TIME_UNAVAILABLE ||
      p->arr_kind<TIME_ESTIMATED || p->arr_kind>TIME_UNAVAILABLE ||
      !valid_field(p->dep_terminal_state) || !valid_field(p->gate_state) ||
      !valid_field(p->arr_terminal_state) || !valid_field(p->baggage_state) ||
      !valid_field(p->booking_state) || !valid_field(p->seat_state)) { return false; }
  if (s->pending_generation!=p->generation) {
    s->pending_generation=p->generation; s->pending_count=p->count; s->received=0;
  }
  if (s->pending_count!=p->count) { return false; }
  NativeFlight *o=&s->pending[p->index]; memset(o,0,sizeof(*o));
  for (int i=0;i<TX_COUNT;i++) { copy(o->text[i],p->text[i]); }
  Flight *f=&o->flight; f->status=p->status; f->progress_percent=p->progress;
  f->timezone_unknown=p->dep_offset==2000 || p->arr_offset==2000;
  f->departure.time.utc_offset_minutes=p->dep_offset;
  f->arrival.time.utc_offset_minutes=p->arr_offset;
  f->departure.time.kind=p->dep_kind; f->arrival.time.kind=p->arr_kind;
  f->departure.time.delay_minutes=p->dep_delay; f->arrival.time.delay_minutes=p->arr_delay;
  f->departure.time.delay_known=p->dep_delay_known; f->arrival.time.delay_known=p->arr_delay_known;
  f->departure.terminal.state=p->dep_terminal_state; f->departure.facility.state=p->gate_state;
  f->arrival.terminal.state=p->arr_terminal_state; f->arrival.facility.state=p->baggage_state;
  f->booking.code.state=p->booking_state; f->booking.seat.state=p->seat_state;
  f->booking.position=p->position; f->booking.position_override=p->position!=SEAT_UNKNOWN;
  o->stale=p->stale; o->loading=p->loading; bind(o);
  s->received|=1U<<p->index;
  if (s->received!=((1U<<p->count)-1U)) { return false; }
  for (int i=0;i<p->count;i++) {
    s->flights[i]=s->pending[i]; bind(&s->flights[i]);
  }
  s->count=p->count; s->generation=p->generation; s->error[0]='\0';
  return true;
}
#endif
