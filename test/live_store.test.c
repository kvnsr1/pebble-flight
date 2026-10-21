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
  p.generation=3; p.count=6; assert(!live_store_apply(&store,&p));
  p.count=1; p.index=4; assert(!live_store_apply(&store,&p));
  p.index=0; p.status=99; assert(!live_store_apply(&store,&p));
  p.count=5; p.status=FLIGHT_IN_FLIGHT; p.twelve_hour=true;
  const char *numbers[]={"AA1","UA2","DL3","WN4","AS5"};
  for (int i=0;i<5;i++) {
    p.index=i;p.text[TX_NUMBER]=numbers[i];p.text[TX_KEY]=numbers[i];
    p.text[TX_DEP_CURRENT]="12:05 AM";
    assert(live_store_apply(&store,&p)==(i==4));
    if (i<4) { assert(store.count==2); }
  }
  assert(store.count==5 && !strcmp(store.flights[4].flight.number,"AS5"));
  assert(store.flights[4].twelve_hour && !strcmp(store.flights[4].flight.departure.time.current,"12:05 AM"));
  p.generation=4;
  p.count=0; p.text[TX_ERROR]="No flights";
  assert(live_store_apply(&store,&p) && store.count==0);
  assert(!strcmp(store.error,"No flights"));
  puts("Native store atomic snapshots, owned strings, stale frames, invalid packets and empty state passed");
}
