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
  if (carrier(number,"AA") || carrier(number,"AAL")) { resource=RESOURCE_ID_AIRLINE_AMERICAN_25; }
  else if (carrier(number,"DL") || carrier(number,"DAL")) { resource=RESOURCE_ID_AIRLINE_DELTA_25; }
  else if (carrier(number,"F9") || carrier(number,"FFT")) { resource=RESOURCE_ID_AIRLINE_FRONTIER_25; }
  else if (carrier(number,"WN") || carrier(number,"SWA")) { resource=RESOURCE_ID_AIRLINE_SOUTHWEST_25; }
  else if (carrier(number,"UA") || carrier(number,"UAL")) { resource=RESOURCE_ID_AIRLINE_UNITED_25; }
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
