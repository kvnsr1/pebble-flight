#include "../prototype_config.h"
#if PEBBLE_UI_PROTOTYPE
#include "flight_model.h"
const char *flight_status_label(FlightStatus status) {
  switch (status) {
    case FLIGHT_UPCOMING: return "Upcoming";
    case FLIGHT_BOARDING: return "Boarding";
    case FLIGHT_IN_FLIGHT: return "In flight";
    case FLIGHT_LANDED: return "Landed";
    case FLIGHT_DELAYED: return "Delayed";
    case FLIGHT_CANCELED: return "Canceled";
    case FLIGHT_DIVERTED: return "Diverted";
    case FLIGHT_DEPARTED: return "Departed";
    case FLIGHT_UNKNOWN: return "Not updated";
  }
  return "Unknown";
}
const char *flight_field_text(FlightField field) {
  if (field.state == FIELD_UNAVAILABLE) { return "N/A"; }
  return field.state == FIELD_KNOWN && field.value ? field.value : "Unknown";
}
int flight_progress(const Flight *flight) {
  if (!flight || flight->status == FLIGHT_CANCELED) { return -1; }
  if (flight->status == FLIGHT_UNKNOWN || flight->status == FLIGHT_DIVERTED) { return -2; }
  if (flight->status == FLIGHT_LANDED) { return 100; }
  if (flight->status != FLIGHT_IN_FLIGHT) { return 0; }
  if (flight->progress_percent < 0) { return -2; }
  return flight->progress_percent < 0 ? 0 :
    (flight->progress_percent > 99 ? 99 : flight->progress_percent);
}
#endif
