#include "../prototype_config.h"
#if PEBBLE_UI_PROTOTYPE
#include "airline_icons.h"
#include <ctype.h>

// Five saved flights can expose several carriers in the same animation frame.
// Retain their decoded PDCs instead of repeatedly replacing a single image.
enum { CACHE_SIZE=5 };
static struct { GDrawCommandImage *image; uint32_t resource, used; } s_icons[CACHE_SIZE];
static uint32_t s_use;

static bool carrier(const char *number, const char *prefix) {
  size_t length=strlen(prefix);
  if (strncmp(number,prefix,length)) { return false; }
  number+=length;
  while (*number==' ') { number++; }
  return isdigit((unsigned char)*number)!=0;
}

static GDrawCommandImage *image_for(const char *number) {
  if (!number) { return NULL; }
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
  if (!resource) { return NULL; }
  unsigned slot=0;
  for (unsigned i=0;i<CACHE_SIZE;i++) {
    if (s_icons[i].resource==resource) {
      s_icons[i].used=++s_use;
      return s_icons[i].image;
    }
    if (s_icons[i].used<s_icons[slot].used) { slot=i; }
  }
  if (s_icons[slot].image) { gdraw_command_image_destroy(s_icons[slot].image); }
  s_icons[slot].resource=resource;
  s_icons[slot].used=++s_use;
  s_icons[slot].image=gdraw_command_image_create_with_resource(resource);
  return s_icons[slot].image;
}
bool airline_icon_draw(GContext *ctx,const char *number,GPoint origin) {
  GDrawCommandImage *image=image_for(number);
  if(!image) { return false; }
  gdraw_command_image_draw(ctx,image,origin);
  return true;
}
GDrawCommandImage *airline_icon_clone(const char *number) {
  GDrawCommandImage *image=image_for(number);
  return image?gdraw_command_image_clone(image):NULL;
}

void airline_icons_deinit(void) {
  for (unsigned i=0;i<CACHE_SIZE;i++) {
    if (s_icons[i].image) { gdraw_command_image_destroy(s_icons[i].image); }
    s_icons[i].image=NULL; s_icons[i].resource=0; s_icons[i].used=0;
  }
  s_use=0;
}
#endif
