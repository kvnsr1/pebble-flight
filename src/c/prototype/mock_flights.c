#include "../prototype_config.h"
#if PEBBLE_UI_PROTOTYPE && PEBBLE_MOCK_DATA
#include "flight_model.h"

// Synthetic, independent examples, frozen in October 2026. These are NOT actual
// airline schedules or operational claims. No clock, network, or credentials.
#define KNOWN(s) { FIELD_KNOWN, s }
#define UNKNOWN { FIELD_UNKNOWN, NULL }
#define NA { FIELD_UNAVAILABLE, NULL }
static const Flight s_flights[] = {
  {
    "AA100", "16 OCT", FLIGHT_UPCOMING, 0,
    { "JFK", "John F. Kennedy International Airport",
      { "18:10", NULL, "16 OCT", "EDT", -240, TIME_UNKNOWN, false, 0, NULL, NULL },
      KNOWN("8"), UNKNOWN },
    { "LHR", "London Heathrow Airport",
      { "06:20", NULL, "17 OCT", "BST", 60, TIME_UNKNOWN, false, 0, NULL, NULL },
      KNOWN("3"), NA },
    { KNOWN("Q7M2KP"), KNOWN("12A"), SEAT_UNKNOWN, false }, false
  },
  {
    "UA206", "16 OCT", FLIGHT_BOARDING, 0,
    { "SFO", "San Francisco International Airport",
      { "09:00", "09:00", "16 OCT", "PDT", -420, TIME_ESTIMATED, true, 0, NULL, NULL },
      KNOWN("3"), KNOWN("F12") },
    { "ORD", "Chicago O'Hare International Airport",
      { "15:10", "15:10", "16 OCT", "CDT", -300, TIME_ESTIMATED, true, 0, NULL, NULL },
      KNOWN("1"), UNKNOWN },
    { KNOWN("R4N8VX"), KNOWN("18B"), SEAT_UNKNOWN, false }, false
  },
  {
    "BA284", "16 OCT", FLIGHT_IN_FLIGHT, 62,
    { "SFO", "San Francisco International Airport",
      { "16:35", "16:47", "16 OCT", "PDT", -420, TIME_ACTUAL, true, 12, NULL, NULL },
      KNOWN("I"), KNOWN("A6") },
    { "LHR", "London Heathrow Airport",
      { "11:10", "10:58", "17 OCT", "BST", 60, TIME_ESTIMATED, true, -12, NULL, NULL },
      KNOWN("5"), UNKNOWN },
    { KNOWN("K8T3WL"), KNOWN("21C"), SEAT_UNKNOWN, false }, false
  },
  {
    "DL418", "16 OCT", FLIGHT_LANDED, 100,
    { "LAX", "Los Angeles International Airport",
      { "07:00", "06:55", "16 OCT", "PDT", -420, TIME_ACTUAL, true, -5, NULL, NULL },
      KNOWN("3"), KNOWN("32B") },
    { "JFK", "John F. Kennedy International Airport",
      { "15:30", "15:48", "16 OCT", "EDT", -240, TIME_ACTUAL, true, 18, NULL, NULL },
      KNOWN("4"), KNOWN("B7") },
    { KNOWN("V2H6FQ"), KNOWN("8C"), SEAT_WINDOW, true }, false
  },
  {
    "AF83", "16 OCT", FLIGHT_DELAYED, 0,
    { "SFO", "San Francisco International Airport",
      { "15:20", "16:35", "16 OCT", "PDT", -420, TIME_ESTIMATED, true, 75, NULL, NULL },
      KNOWN("I"), UNKNOWN },
    { "CDG", "Paris Charles de Gaulle International Airport",
      { "11:15", "12:30", "17 OCT", "CEST", 120, TIME_ESTIMATED, true, 75, NULL, NULL },
      KNOWN("2E"), NA },
    { KNOWN("DEMO12345678"), UNKNOWN, SEAT_UNKNOWN, false }, false
  },
  {
    "EK202", "16 OCT", FLIGHT_CANCELED, 0,
    { "JFK", "John F. Kennedy International Airport",
      { "23:00", NULL, "16 OCT", "EDT", -240, TIME_UNAVAILABLE, false, 0, NULL, NULL },
      NA, NA },
    { "DXB", "Dubai International Airport",
      { "20:00", NULL, "17 OCT", "GST", 240, TIME_UNAVAILABLE, false, 0, NULL, NULL },
      NA, NA },
    { NA, NA, SEAT_UNKNOWN, false }, false
  }
};

size_t mock_flight_count(void) { return sizeof(s_flights) / sizeof(s_flights[0]); }
const Flight *mock_flight_at(size_t index) {
  return index < mock_flight_count() ? &s_flights[index] : NULL;
}
#endif
