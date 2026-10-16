#include "../prototype_config.h"
#if PEBBLE_UI_PROTOTYPE
#include "flight_model.h"
#include <string.h>

SeatPosition booking_seat_position(const FlightBooking *booking) {
  if (booking->position_override) { return booking->position; }
  if (booking->seat.state != FIELD_KNOWN || !booking->seat.value) { return SEAT_UNKNOWN; }
  const char *seat = booking->seat.value;
  size_t length = strlen(seat);
  // Same supported range as the live phone app: rows 1-99, letters A-F.
  // Other aircraft layouts require an explicit saved position; never guess.
  if (length < 2 || length > 3 || seat[0] < '1' || seat[0] > '9' ||
      (length == 3 && (seat[1] < '0' || seat[1] > '9'))) { return SEAT_UNKNOWN; }
  char letter = seat[length - 1];
  if (letter >= 'a' && letter <= 'f') { letter -= 'a' - 'A'; }
  switch (letter) {
    case 'A': case 'F': return SEAT_WINDOW;
    case 'B': case 'E': return SEAT_MIDDLE;
    case 'C': case 'D': return SEAT_AISLE;
    default: return SEAT_UNKNOWN;
  }
}
const char *booking_position_label(const FlightBooking *booking) {
  switch (booking_seat_position(booking)) {
    case SEAT_WINDOW: return "Window seat";
    case SEAT_MIDDLE: return "Middle seat";
    case SEAT_AISLE: return "Aisle seat";
    default: return booking->seat.state == FIELD_UNAVAILABLE ? "Position N/A" : "Position unknown";
  }
}
#endif
