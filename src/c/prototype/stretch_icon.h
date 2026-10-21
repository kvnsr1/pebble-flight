#pragma once
#include <pebble.h>
typedef struct StretchIcon StretchIcon;
// Takes ownership of image, including on failure. Only the clone is modified.
StretchIcon *stretch_icon_create(GDrawCommandImage *image);
void stretch_icon_destroy(StretchIcon *icon);
bool stretch_icon_draw(StretchIcon *icon,GContext *ctx,GRect box,int progress,bool reverse);
