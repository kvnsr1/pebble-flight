#pragma once
#include <stdbool.h>
#include <stddef.h>

// Provider-neutral view model. No FlightAware schema or phone storage here.
typedef enum { FIELD_KNOWN, FIELD_UNKNOWN, FIELD_UNAVAILABLE } FieldState;
typedef struct { FieldState state; const char *value; } FlightField;
typedef enum { TIME_ESTIMATED, TIME_ACTUAL, TIME_UNKNOWN, TIME_UNAVAILABLE } TimeKind;
typedef struct {
  const char *scheduled;
  const char *current;
  const char *date;        // Airport-local date, including any overnight change.
  const char *zone;        // Explicit abbreviation; offset disambiguates it.
  int utc_offset_minutes;  // Offset at this flight's local date, not watch time.
  TimeKind kind;
  bool delay_known;
  int delay_minutes;       // Signed: negative is early, positive is late.
  const char *scheduled_date; // Only needed when schedule and current date differ.
  const char *scheduled_zone; // Explicit schedule zone when DST/offset changes.
} FlightTime;
typedef struct {
  const char *code;
  const char *name;
  FlightTime time;
  FlightField terminal;
  FlightField facility;    // Departure gate or arrival baggage claim.
} FlightAirport;
typedef enum {
  FLIGHT_UPCOMING, FLIGHT_BOARDING, FLIGHT_IN_FLIGHT,
  FLIGHT_LANDED, FLIGHT_DELAYED, FLIGHT_CANCELED,
  FLIGHT_DIVERTED, FLIGHT_DEPARTED, FLIGHT_UNKNOWN
} FlightStatus;
typedef enum { SEAT_UNKNOWN, SEAT_WINDOW, SEAT_MIDDLE, SEAT_AISLE } SeatPosition;
typedef struct {
  FlightField code;
  FlightField seat;
  SeatPosition position;
  bool position_override; // Saved phone setting takes precedence over auto A-F.
} FlightBooking;
typedef struct {
  const char *number;
  const char *date;        // Departure date shown in All Flights.
  FlightStatus status;
  int progress_percent;    // Explicit data, never calculated from local strings.
  FlightAirport departure;
  FlightAirport arrival;
  FlightBooking booking;
  bool timezone_unknown;
} Flight;

const Flight *mock_flight_at(size_t index);
size_t mock_flight_count(void);
const char *flight_status_label(FlightStatus status);
const char *flight_field_text(FlightField field);
int flight_progress(const Flight *flight); // -1 = no journey (canceled).
SeatPosition booking_seat_position(const FlightBooking *booking);
const char *booking_position_label(const FlightBooking *booking);
