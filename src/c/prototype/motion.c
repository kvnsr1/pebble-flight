#include "motion.h"

FlightHeaderPosition motion_flight_header(int row_y, int number_width, int progress) {
#if !PEBBLE_STRETCH_MOTION
  int p=motion_ease(progress),vertical=motion_ease(progress*2),horizontal=motion_ease((progress-500)*2);
  FlightHeaderPosition position={.logo_x=motion_mix(12,86,p),.logo_y=motion_mix(row_y+3,66,p),.logo_size=25,.number_scale=1000};
  position.number_x=position.logo_x+motion_mix(32,12-number_width/2,horizontal);
  position.number_y=position.logo_y+motion_mix(-1,25,vertical);
  return position;
#else
  int p=motion_smooth(progress);
  int vertical=motion_smooth(motion_phase(progress,0,350));
  int horizontal=motion_smooth(motion_phase(progress,350,950));
  int pulse=motion_pulse(progress);
  FlightHeaderPosition position;
  position.logo_size=25+MOTION_HERO_ICON_GROW*pulse/MOTION_UNIT;
  position.number_scale=1000+MOTION_HERO_TEXT_GROW*motion_pulse(motion_phase(progress,60,1000))/MOTION_UNIT;
  int x=motion_mix(12,86,p), y=motion_mix(row_y+3,66,p);
  position.logo_x=x-(position.logo_size-25)/2;
  position.logo_y=y-(position.logo_size-25)/2;
  int width=number_width*position.number_scale/MOTION_UNIT;
  position.number_x=motion_mix(x+32,98-width/2,horizontal);
  position.number_y=y+motion_mix(-1,25,vertical)+(position.logo_size-25)*vertical/MOTION_UNIT;
  // Clear the full stretched icon before the number centers beneath it.
  int gap=3*pulse/MOTION_UNIT;
  if (position.number_y<position.logo_y+position.logo_size+gap &&
      position.number_x<position.logo_x+position.logo_size+4) {
    position.number_x=position.logo_x+position.logo_size+4;
  }
  return position;
#endif
}

static int clamp(int n) { return n < 0 ? 0 : n > MOTION_UNIT ? MOTION_UNIT : n; }

int motion_phase(int p,int start,int end) {
  return end>start?clamp((int)((int64_t)(p-start)*MOTION_UNIT/(end-start))):p>=end?MOTION_UNIT:0;
}
int motion_smooth(int p) {
  p=clamp(p);return (int)((int64_t)p*p*(3000-2*p)/1000000);
}
int motion_pulse(int p) {
  p=clamp(p);return motion_smooth(p<450?p*1000/450:(1000-p)*1000/550);
}
int motion_band(int coordinate,int extent,int reverse) {
  int band=extent>0?(int)((int64_t)coordinate*MOTION_BANDS/extent):0;
  if(band<0)band=0;
  if(band>=MOTION_BANDS)band=MOTION_BANDS-1;
  return reverse?MOTION_BANDS-1-band:band;
}
int motion_band_offset(int progress,int band) {
  int local=motion_smooth(motion_phase(progress,band*MOTION_BAND_STAGGER,1000-(MOTION_BANDS-1-band)*MOTION_BAND_STAGGER));
  // Bound the point lag to three pixels; retain each airline's silhouette.
  int delta=(local-motion_smooth(progress))*18/1000;
  return delta<-MOTION_BAND_LIMIT?-MOTION_BAND_LIMIT:delta>MOTION_BAND_LIMIT?MOTION_BAND_LIMIT:delta;
}

int motion_mix(int from, int to, int progress) {
  return from + (int)((int64_t)(to - from) * clamp(progress) / MOTION_UNIT);
}

int motion_ease(int progress) {
  int remaining = MOTION_UNIT - clamp(progress);
  return MOTION_UNIT - (int)((int64_t)remaining * remaining * remaining / 1000000);
}

// One small overshoot and one settle; integers keep the original art pixel aligned.
int motion_travel(int distance, int overshoot, int progress) {
  int p = clamp(progress);
  if (p < 780) {
    return motion_mix(0, distance + overshoot, motion_ease(p * MOTION_UNIT / 780));
  }
  return motion_mix(distance + overshoot, distance, (p - 780) * MOTION_UNIT / 220);
}
