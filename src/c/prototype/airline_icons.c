#include "../prototype_config.h"
#if PEBBLE_UI_PROTOTYPE
#include "airline_icons.h"
#include <ctype.h>

static GDrawCommandImage *s_icon;
static uint32_t s_resource;

static bool carrier(const char *number, const char *prefix) {
  size_t length=strlen(prefix);
  if (strncmp(number,prefix,length)) { return false; }
  number+=length;
  while (*number==' ') { number++; }
  return isdigit((unsigned char)*number)!=0;
}

bool airline_icon_draw(GContext *ctx, const char *number, GPoint origin) {
  if (!number) { return false; }
  uint32_t resource=0;
  static const struct { const char *iata, *icao; uint32_t resource; } airlines[]={
    {"CA","CCA",RESOURCE_ID_AIRLINE_AIR_CHINA_25},
    {"AF","AFR",RESOURCE_ID_AIRLINE_AIR_FRANCE_25},
    {"AS","ASA",RESOURCE_ID_AIRLINE_ALASKA_25},
    {"AA","AAL",RESOURCE_ID_AIRLINE_AMERICAN_25},
    {"CX","CPA",RESOURCE_ID_AIRLINE_CATHAY_PACIFIC_25},
    {"MU","CES",RESOURCE_ID_AIRLINE_CHINA_EASTERN_25},
    {"DL","DAL",RESOURCE_ID_AIRLINE_DELTA_25},
    {"F9","FFT",RESOURCE_ID_AIRLINE_FRONTIER_25},
    {"HA","HAL",RESOURCE_ID_AIRLINE_HAWAIIAN_25},
    {"XE","JSX",RESOURCE_ID_AIRLINE_JSX_25},
    {"KL","KLM",RESOURCE_ID_AIRLINE_KLM_25},
    {"KE","KAL",RESOURCE_ID_AIRLINE_KOREAN_AIR_25},
    {"LA","LAN",RESOURCE_ID_AIRLINE_LATAM_25},
    {"LH","DLH",RESOURCE_ID_AIRLINE_LUFTHANSA_25},
    {"WN","SWA",RESOURCE_ID_AIRLINE_SOUTHWEST_25},
    {"TK","THY",RESOURCE_ID_AIRLINE_TURKISH_25},
    {"UA","UAL",RESOURCE_ID_AIRLINE_UNITED_25},
  };
  for (unsigned i=0;i<ARRAY_LENGTH(airlines);i++) {
    if (carrier(number,airlines[i].iata) || carrier(number,airlines[i].icao)) {
      resource=airlines[i].resource; break;
    }
  }
  if (!resource) { return false; }
  // Keep just the current carrier in RAM, and release it when switching carriers.
  if (resource!=s_resource) {
    airline_icons_deinit();
    s_icon=gdraw_command_image_create_with_resource(resource);
    if (s_icon) { s_resource=resource; }
  }
  if (!s_icon) { return false; }
  gdraw_command_image_draw(ctx,s_icon,origin);
  return true;
}

void airline_icons_deinit(void) {
  if (s_icon) { gdraw_command_image_destroy(s_icon); s_icon=NULL; }
  s_resource=0;
}
#endif
