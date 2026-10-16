#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/c/prototype/live_store.h"
static LiveStore store;
int main(void) {
  FlightPacket p={0}; p.count=2; p.index=0; p.generation=1;
  p.status=FLIGHT_IN_FLIGHT; p.progress=62; p.dep_kind=TIME_ACTUAL; p.arr_kind=TIME_ESTIMATED;
  p.position=SEAT_WINDOW; p.dep_delay_known=1; p.dep_delay=12; p.arr_delay_known=1; p.arr_delay=-12;
  p.text[TX_KEY]="BA284|2026-10-16"; p.text[TX_NUMBER]="BA284"; p.text[TX_DATE]="16 OCT";
  p.text[TX_DEP_CURRENT]="16:47"; p.text[TX_BOOKING]="ABC123"; p.text[TX_SEAT]="8C";
  assert(!live_store_apply(&store,&p) && store.count==0);
  p.index=1; p.text[TX_KEY]="AA100|2026-10-16"; p.text[TX_NUMBER]="AA100";
  assert(live_store_apply(&store,&p) && store.count==2);
  assert(!strcmp(store.flights[0].flight.number,"BA284"));
  assert(!strcmp(store.flights[1].flight.number,"AA100"));
  assert(!strcmp(store.flights[0].flight.booking.code.value,"ABC123"));
  assert(store.flights[0].flight.booking.position_override);
  // Incomplete snapshots cannot overwrite displayed strings (owned pointers).
  p.generation=2; p.index=0; p.text[TX_NUMBER]="NEW1";
  assert(!live_store_apply(&store,&p));
  assert(!strcmp(store.flights[0].flight.number,"BA284"));
  p.generation=1; p.index=1;
  assert(!live_store_apply(&store,&p));
  p.generation=2; p.index=1; p.text[TX_NUMBER]="NEW2";
  assert(live_store_apply(&store,&p));
  assert(!strcmp(store.flights[0].flight.number,"NEW1"));
  assert(!strcmp(store.flights[1].flight.number,"NEW2"));
  p.generation=3; p.count=4; assert(!live_store_apply(&store,&p));
  p.count=1; p.index=4; assert(!live_store_apply(&store,&p));
  p.index=0; p.status=99; assert(!live_store_apply(&store,&p));
  p.count=0; p.text[TX_ERROR]="No flights";
  assert(live_store_apply(&store,&p) && store.count==0);
  assert(!strcmp(store.error,"No flights"));
  puts("Native store atomic snapshots, owned strings, stale frames, invalid packets and empty state passed");
}
