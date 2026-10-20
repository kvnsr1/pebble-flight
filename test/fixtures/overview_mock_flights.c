// Preview-only data. scripts/build-overview-mockup.py copies this over the mock
// fixture in an isolated project. These are NOT real routes/schedules/bookings.
#include "../prototype_config.h"
#if PEBBLE_UI_PROTOTYPE && PEBBLE_MOCK_DATA
#include "flight_model.h"
#define KNOWN(s) { FIELD_KNOWN, s }
#define UNKNOWN { FIELD_UNKNOWN, NULL }
#define NA { FIELD_UNAVAILABLE, NULL }
static const struct { const char *number; FlightStatus status; int progress; } s_cases[]={
  {"WN1145",FLIGHT_UPCOMING,0}, {"AA2220",FLIGHT_IN_FLIGHT,45},
  {"DL418",FLIGHT_LANDED,100}, {"UA206",FLIGHT_BOARDING,0},
  {"F9100",FLIGHT_DELAYED,0}, {"AS100",FLIGHT_IN_FLIGHT,35},
  {"CA100",FLIGHT_CANCELED,0}, {"CX100",FLIGHT_IN_FLIGHT,65},
  {"XE100",FLIGHT_UPCOMING,0}, {"LA100",FLIGHT_IN_FLIGHT,75},
  {"HA100",FLIGHT_IN_FLIGHT,25}, {"KE100",FLIGHT_IN_FLIGHT,50},
  {"LH100",FLIGHT_IN_FLIGHT,90}, {"AF100",FLIGHT_DELAYED,0},
  {"KL100",FLIGHT_LANDED,100}, {"TK100",FLIGHT_BOARDING,0},
  {"MU100",FLIGHT_IN_FLIGHT,60}, {"BA284",FLIGHT_IN_FLIGHT,-1}
};
static const Flight s_base={
  "WN1145","19 OCT",FLIGHT_UPCOMING,0,
  {"BUR","Hollywood Burbank Airport",
    {"20:20","20:20","19 OCT","PDT",-420,TIME_ESTIMATED,true,0,NULL,NULL},
    KNOWN("A"),UNKNOWN},
  {"OAK","Oakland International Airport",
    {"21:40","21:40","19 OCT","PDT",-420,TIME_ESTIMATED,true,0,NULL,NULL},
    KNOWN("2"),NA},
  {KNOWN("DEMO123"),KNOWN("12A"),SEAT_WINDOW,true},false
};
static Flight s_flights[sizeof(s_cases)/sizeof(s_cases[0])];
static bool s_ready;
size_t mock_flight_count(void) { return sizeof(s_cases)/sizeof(s_cases[0]); }
const Flight *mock_flight_at(size_t index) {
  if (index>=mock_flight_count()) { return NULL; }
  if (!s_ready) {
    for (size_t i=0;i<mock_flight_count();i++) {
      s_flights[i]=s_base;
      s_flights[i].number=s_cases[i].number;
      s_flights[i].status=s_cases[i].status;
      s_flights[i].progress_percent=s_cases[i].progress;
      if (s_cases[i].status==FLIGHT_DELAYED) {
        s_flights[i].departure.time.current="21:35";
        s_flights[i].departure.time.delay_minutes=75;
      }
      if (s_cases[i].status==FLIGHT_CANCELED) {
        s_flights[i].departure.time.kind=TIME_UNAVAILABLE;
        s_flights[i].arrival.time.kind=TIME_UNAVAILABLE;
      }
      if (s_cases[i].status==FLIGHT_IN_FLIGHT || s_cases[i].status==FLIGHT_LANDED) {
        s_flights[i].departure.time.kind=TIME_ACTUAL;
      }
      if (s_cases[i].status==FLIGHT_LANDED) { s_flights[i].arrival.time.kind=TIME_ACTUAL; }
    }
    s_ready=true;
  }
  return &s_flights[index];
}
#endif
