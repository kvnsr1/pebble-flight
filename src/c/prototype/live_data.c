#include "../prototype_config.h"
#if PEBBLE_UI_PROTOTYPE && !PEBBLE_MOCK_DATA
#include <pebble.h>
#include "live_data.h"
#include "watch_glance.h"
static LiveStore s_store;
static void (*s_changed)(void);
static AppTimer *s_heartbeat;
static AppTimer *s_connect_timer;
static bool s_phone_ready;

static const char *string(DictionaryIterator *iter,uint32_t key) {
  Tuple *t=dict_find(iter,key); return t && t->type==TUPLE_CSTRING?t->value->cstring:"";
}
static int integer(DictionaryIterator *iter,uint32_t key,int fallback) {
  Tuple *t=dict_find(iter,key);
  return t && (t->type==TUPLE_UINT || t->type==TUPLE_INT)?(int)t->value->int32:fallback;
}
static void mode_hint(void *context) {
  s_connect_timer=NULL;
  if (s_phone_ready) { return; }
  if (!s_phone_ready && connection_service_peek_pebble_app_connection()) {
    DictionaryIterator *out;
    if (app_message_outbox_begin(&out)==APP_MSG_OK) {
      dict_write_uint8(out,MESSAGE_KEY_LIVE_MODE,2); app_message_outbox_send();
    }
  }
  s_connect_timer=app_timer_register(5000,mode_hint,NULL);
}
static void connection(bool connected) {
  if (!connected) {
    s_phone_ready=false;
    if (!s_connect_timer) { s_connect_timer=app_timer_register(5000,mode_hint,NULL); }
  }
  if (s_changed) { s_changed(); }
}
static void inbox(DictionaryIterator *iter,void *context) {
  if (dict_find(iter,MESSAGE_KEY_REQUEST_MODE)) {
    DictionaryIterator *out;
    if (app_message_outbox_begin(&out)==APP_MSG_OK) {
      dict_write_uint8(out,MESSAGE_KEY_LIVE_MODE,2); app_message_outbox_send();
    }
    return;
  }
  if (dict_find(iter,MESSAGE_KEY_GLANCE_PLAN)) {
    watch_glance_update(string(iter,MESSAGE_KEY_GLANCE_PLAN));
    return;
  }
  if (!dict_find(iter,MESSAGE_KEY_UI_COUNT)) { return; }
  const uint32_t text_keys[TX_COUNT]={
    MESSAGE_KEY_UI_KEY,MESSAGE_KEY_FLIGHT_NUMBER,MESSAGE_KEY_FLIGHT_DATE,
    MESSAGE_KEY_ORIGIN,MESSAGE_KEY_DESTINATION,MESSAGE_KEY_UI_DEP_NAME,MESSAGE_KEY_UI_ARR_NAME,
    MESSAGE_KEY_UI_DEP_SCHEDULED,MESSAGE_KEY_UI_ARR_SCHEDULED,MESSAGE_KEY_DEPARTURE_TIME,MESSAGE_KEY_ARRIVAL_TIME,
    MESSAGE_KEY_UI_DEP_DATE,MESSAGE_KEY_UI_ARR_DATE,MESSAGE_KEY_UI_DEP_ZONE,MESSAGE_KEY_UI_ARR_ZONE,
    MESSAGE_KEY_UI_DEP_SCHEDULED_DATE,MESSAGE_KEY_UI_ARR_SCHEDULED_DATE,
    MESSAGE_KEY_UI_DEP_SCHEDULED_ZONE,MESSAGE_KEY_UI_ARR_SCHEDULED_ZONE,
    MESSAGE_KEY_DEPARTURE_TERMINAL,MESSAGE_KEY_ARRIVAL_TERMINAL,MESSAGE_KEY_DEPARTURE_GATE,MESSAGE_KEY_UI_BAGGAGE,
    MESSAGE_KEY_BOOKING_CODE,MESSAGE_KEY_SEAT_NUMBER,MESSAGE_KEY_UI_UPDATED,MESSAGE_KEY_ERROR_MESSAGE,
    MESSAGE_KEY_AIRCRAFT_MODEL,MESSAGE_KEY_AIRCRAFT_NUMBER,MESSAGE_KEY_AIRCRAFT_FIRST_FLIGHT,
    MESSAGE_KEY_AIRCRAFT_LEG_1,MESSAGE_KEY_AIRCRAFT_LEG_1_STATUS,MESSAGE_KEY_AIRCRAFT_LEG_2,
    MESSAGE_KEY_AIRCRAFT_LEG_2_STATUS,MESSAGE_KEY_NEXT_REFRESH_AT
  };
  FlightPacket p={0};
  p.count=integer(iter,MESSAGE_KEY_UI_COUNT,-1); p.index=integer(iter,MESSAGE_KEY_UI_INDEX,-1);
  p.generation=integer(iter,MESSAGE_KEY_UI_GENERATION,0);
  p.status=integer(iter,MESSAGE_KEY_UI_STATUS,FLIGHT_UNKNOWN);
  p.progress=integer(iter,MESSAGE_KEY_UI_PROGRESS,-1);
  p.dep_offset=integer(iter,MESSAGE_KEY_UI_DEP_OFFSET,2000); p.arr_offset=integer(iter,MESSAGE_KEY_UI_ARR_OFFSET,2000);
  p.dep_kind=integer(iter,MESSAGE_KEY_UI_DEP_KIND,TIME_UNKNOWN); p.arr_kind=integer(iter,MESSAGE_KEY_UI_ARR_KIND,TIME_UNKNOWN);
  p.dep_delay=integer(iter,MESSAGE_KEY_UI_DEP_DELAY,0); p.arr_delay=integer(iter,MESSAGE_KEY_UI_ARR_DELAY,0);
  p.dep_delay_known=integer(iter,MESSAGE_KEY_UI_DEP_DELAY_KNOWN,0); p.arr_delay_known=integer(iter,MESSAGE_KEY_UI_ARR_DELAY_KNOWN,0);
  p.dep_terminal_state=integer(iter,MESSAGE_KEY_UI_DEP_TERMINAL_STATE,FIELD_UNKNOWN);
  p.arr_terminal_state=integer(iter,MESSAGE_KEY_UI_ARR_TERMINAL_STATE,FIELD_UNKNOWN);
  p.gate_state=integer(iter,MESSAGE_KEY_UI_DEP_GATE_STATE,FIELD_UNKNOWN);
  p.baggage_state=integer(iter,MESSAGE_KEY_UI_BAGGAGE_STATE,FIELD_UNAVAILABLE);
  p.booking_state=integer(iter,MESSAGE_KEY_UI_BOOKING_STATE,FIELD_UNKNOWN);
  p.seat_state=integer(iter,MESSAGE_KEY_UI_SEAT_STATE,FIELD_UNKNOWN);
  const char *position=string(iter,MESSAGE_KEY_SEAT_POSITION);
  p.position=!strcmp(position,"WINDOW")?SEAT_WINDOW:!strcmp(position,"MIDDLE")?SEAT_MIDDLE:
             !strcmp(position,"AISLE")?SEAT_AISLE:SEAT_UNKNOWN;
  p.stale=integer(iter,MESSAGE_KEY_UI_STALE,1); p.loading=integer(iter,MESSAGE_KEY_IS_LOADING,0);
  for (int i=0;i<TX_COUNT;i++) { p.text[i]=string(iter,text_keys[i]); }
  if (live_store_apply(&s_store,&p)) { s_phone_ready=true; if (s_changed) { s_changed(); } }
}
static void failed(AppMessageResult result,void *context) {
  s_phone_ready=false;
  if (!s_connect_timer) { s_connect_timer=app_timer_register(5000,mode_hint,NULL); }
  snprintf(s_store.error,sizeof(s_store.error),"Phone unavailable. Showing saved data.");
  if (s_changed) { s_changed(); }
}
static void outbox_failed(DictionaryIterator *iter,AppMessageResult result,void *context) { failed(result,context); }
void live_select_flight(size_t index,bool manual) {
  DictionaryIterator *out;
  if (app_message_outbox_begin(&out)==APP_MSG_OK) {
    dict_write_uint8(out,MESSAGE_KEY_REQUEST_SELECT_FLIGHT,index);
    dict_write_uint8(out,MESSAGE_KEY_REQUEST_REFRESH,manual?1:2);
    app_message_outbox_send();
  } else { failed(APP_MSG_BUSY,NULL); }
}
void live_refresh(void) {
  DictionaryIterator *out;
  if (app_message_outbox_begin(&out)==APP_MSG_OK) {
    dict_write_uint8(out,MESSAGE_KEY_REQUEST_REFRESH,2); app_message_outbox_send();
  }
}
void live_aircraft(size_t index) {
  DictionaryIterator *out;
  if (app_message_outbox_begin(&out)==APP_MSG_OK) {
    dict_write_uint8(out,MESSAGE_KEY_REQUEST_SELECT_FLIGHT,index);
    dict_write_uint8(out,MESSAGE_KEY_REQUEST_AIRCRAFT_DETAILS,1); app_message_outbox_send();
  }
}
static void heartbeat(void *context) {
  if (s_phone_ready) { live_refresh(); }
  s_heartbeat=app_timer_register(5*60*1000,heartbeat,NULL);
}
void live_data_init(void (*changed)(void)) {
  s_changed=changed;
  snprintf(s_store.error,sizeof(s_store.error),"Connecting to phone...");
  app_message_register_inbox_received(inbox); app_message_register_outbox_failed(outbox_failed); app_message_register_inbox_dropped(failed);
  app_message_open(4096,128);
  connection_service_subscribe((ConnectionHandlers){.pebble_app_connection_handler=connection});
  s_heartbeat=app_timer_register(5*60*1000,heartbeat,NULL);
  s_connect_timer=app_timer_register(5000,mode_hint,NULL);
}
void live_data_deinit(void) {
  if (s_heartbeat) { app_timer_cancel(s_heartbeat); }
  if (s_connect_timer) { app_timer_cancel(s_connect_timer); }
  app_message_deregister_callbacks(); connection_service_unsubscribe();
}
size_t live_flight_count(void) { return s_store.count; }
const Flight *live_flight_at(size_t index) { return index<(size_t)s_store.count?&s_store.flights[index].flight:NULL; }
const NativeFlight *live_metadata_at(size_t index) { return index<(size_t)s_store.count?&s_store.flights[index]:NULL; }
const char *live_global_error(void) { return s_store.error; }
#endif
