#pragma once
#include "flight_model.h"
#include <stdint.h>

enum { LIVE_FLIGHT_LIMIT=3, LIVE_TEXT_SIZE=96 };
typedef enum {
  TX_KEY,TX_NUMBER,TX_DATE,TX_ORIGIN,TX_DESTINATION,TX_DEP_NAME,TX_ARR_NAME,
  TX_DEP_SCHEDULED,TX_ARR_SCHEDULED,TX_DEP_CURRENT,TX_ARR_CURRENT,
  TX_DEP_DATE,TX_ARR_DATE,TX_DEP_ZONE,TX_ARR_ZONE,TX_DEP_SCHEDULED_DATE,TX_ARR_SCHEDULED_DATE,TX_DEP_SCHEDULED_ZONE,TX_ARR_SCHEDULED_ZONE,
  TX_DEP_TERMINAL,TX_ARR_TERMINAL,TX_GATE,TX_BAGGAGE,TX_BOOKING,TX_SEAT,
  TX_UPDATED,TX_ERROR,TX_AIRCRAFT_MODEL,TX_AIRCRAFT_NUMBER,TX_AIRCRAFT_FIRST,
  TX_LEG1,TX_LEG1_STATUS,TX_LEG2,TX_LEG2_STATUS,TX_NEXT_REFRESH,TX_COUNT
} LiveText;
typedef struct {
  uint32_t generation;
  int count,index,status,progress,dep_offset,arr_offset,dep_kind,arr_kind;
  int dep_delay,arr_delay,dep_delay_known,arr_delay_known;
  int dep_terminal_state,gate_state,arr_terminal_state,baggage_state,booking_state,seat_state;
  SeatPosition position;
  bool stale,loading;
  const char *text[TX_COUNT];
} FlightPacket;
typedef struct {
  Flight flight;
  char text[TX_COUNT][LIVE_TEXT_SIZE];
  bool stale,loading;
} NativeFlight;
typedef struct {
  NativeFlight flights[LIVE_FLIGHT_LIMIT],pending[LIVE_FLIGHT_LIMIT];
  uint32_t generation,pending_generation;
  int count,pending_count;
  unsigned received;
  char error[LIVE_TEXT_SIZE];
} LiveStore;
// Commits only complete validated snapshots. Partial/old frames cannot replace
// the list, misattribute another flight's response or dangle string pointers.
bool live_store_apply(LiveStore *store,const FlightPacket *packet);
