#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/c/prototype/flight_model.h"
#include "../src/c/prototype/navigation.h"

int main(void) {
  size_t count = mock_flight_count();
  assert(count == 6 && mock_flight_at(count) == NULL);
  bool seen[6] = {false};
  for (size_t i=0; i<count; i++) {
    const Flight *f = mock_flight_at(i);
    seen[f->status] = true;
    assert(f->departure.time.scheduled && f->arrival.time.scheduled);
    assert(f->departure.time.zone && f->arrival.time.zone);
    int p = flight_progress(f);
    assert(p >= -1 && p <= 100);
    assert(f->status != FLIGHT_CANCELED || p == -1);
    assert(f->status != FLIGHT_LANDED || p == 100);
    assert(f->status != FLIGHT_UPCOMING || p == 0);
  }
  for (int i=0; i<6; i++) { assert(seen[i]); }
  assert(flight_progress(mock_flight_at(2)) == 62);
  assert(mock_flight_at(2)->arrival.time.delay_minutes == -12);
  assert(mock_flight_at(4)->departure.time.delay_minutes == 75);
  assert(strcmp(mock_flight_at(0)->departure.time.date, mock_flight_at(0)->arrival.time.date) != 0);
  assert(strcmp(flight_field_text((FlightField){FIELD_UNKNOWN,NULL}), "Unknown") == 0);
  assert(strcmp(flight_field_text((FlightField){FIELD_UNAVAILABLE,NULL}), "N/A") == 0);
  assert(strcmp(flight_field_text((FlightField){FIELD_KNOWN,"B7"}), "B7") == 0);
  PrototypeNavigation nav = {.page = PAGE_LIST};
  navigation_move(&nav,-1,count);
  assert(nav.selected == 0 && nav.top == 0);
  for (int i=0; i<20; i++) { navigation_move(&nav,1,count); }
  assert(nav.selected == 5 && nav.top == 3);
  navigation_select(&nav,count);
  assert(nav.page == PAGE_OVERVIEW);
  navigation_move(&nav,-1,count);
  assert(nav.page == PAGE_OVERVIEW);
  navigation_move(&nav,1,count);
  assert(nav.page == PAGE_DEPARTURE);
  navigation_move(&nav,1,count);
  navigation_move(&nav,1,count);
  assert(nav.page == PAGE_ARRIVAL);
  navigation_move(&nav,-1,count);
  assert(nav.page == PAGE_DEPARTURE);
  navigation_select(&nav,count);
  assert(nav.note);
  assert(!navigation_back(&nav));
  assert(nav.page == PAGE_LIST && nav.selected == 5 && nav.top == 3 && !nav.note);
  navigation_toggle_empty(&nav);
  navigation_select(&nav,count);
  navigation_move(&nav,-1,count);
  assert(nav.page == PAGE_LIST && nav.empty && nav.selected == 5 && nav.top == 3);
  navigation_toggle_empty(&nav);
  assert(!nav.empty && nav.selected == 5 && nav.top == 3);
  navigation_move(&nav,-1,count);
  navigation_select(&nav,count);
  assert(nav.selected == 4 && nav.page == PAGE_OVERVIEW);
  navigation_select(&nav,count);
  navigation_move(&nav,1,count);
  assert(!nav.note && nav.page == PAGE_DEPARTURE);
  assert(!navigation_back(&nav));
  assert(navigation_back(&nav));
  puts("Mock scenarios, progress, navigation boundaries, selection, scroll and empty state passed");
}
