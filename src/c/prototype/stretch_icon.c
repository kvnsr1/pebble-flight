#include "../prototype_config.h"
#if PEBBLE_UI_PROTOTYPE
#include "stretch_icon.h"
#include "motion.h"
typedef struct {
  GDrawCommand *command;
  uint16_t first,count,radius;
  uint8_t units,stroke;
} SavedCommand;
struct StretchIcon {
  GDrawCommandImage *image;
  GSize bounds;
  unsigned count;
  SavedCommand *commands;
  GPoint *points;
};
void stretch_icon_destroy(StretchIcon *icon) {
  if(!icon)return;
  if(icon->image)gdraw_command_image_destroy(icon->image);
  free(icon->commands);free(icon->points);free(icon);
}
StretchIcon *stretch_icon_create(GDrawCommandImage *image) {
  if(!image)return NULL;
  StretchIcon *icon=calloc(1,sizeof(*icon));
  if(!icon){gdraw_command_image_destroy(image);return NULL;}
  icon->image=image;icon->bounds=gdraw_command_image_get_bounds_size(image);
  GDrawCommandList *list=gdraw_command_image_get_command_list(image);
  icon->count=gdraw_command_list_get_num_commands(list);
  if(!icon->count || icon->count>128 || icon->bounds.w<=0 || icon->bounds.h<=0)goto fail;
  unsigned total=0;
  for(unsigned i=0;i<icon->count;i++)total+=gdraw_command_get_num_points(gdraw_command_list_get_command(list,i));
  if(!total || total>2048)goto fail;
  icon->commands=calloc(icon->count,sizeof(*icon->commands));
  icon->points=malloc(total*sizeof(*icon->points));
  if(!icon->commands || !icon->points)goto fail;
  unsigned at=0;
  for(unsigned i=0;i<icon->count;i++) {
    SavedCommand *s=&icon->commands[i];s->command=gdraw_command_list_get_command(list,i);
    s->first=at;s->count=gdraw_command_get_num_points(s->command);
    s->units=gdraw_command_get_type(s->command)==GDrawCommandTypePrecisePath?8:1;
    s->radius=gdraw_command_get_type(s->command)==GDrawCommandTypeCircle?gdraw_command_get_radius(s->command):0;
    s->stroke=gdraw_command_get_stroke_width(s->command);
    for(unsigned j=0;j<s->count;j++)icon->points[at++]=gdraw_command_get_point(s->command,j);
  }
  return icon;
fail:stretch_icon_destroy(icon);return NULL;
}
bool stretch_icon_draw(StretchIcon *icon,GContext *ctx,GRect box,int progress,bool reverse) {
  if(!icon)return false;
  for(unsigned i=0;i<icon->count;i++) {
    SavedCommand *s=&icon->commands[i];int units=s->units;
    for(unsigned j=0;j<s->count;j++) {
      GPoint original=icon->points[s->first+j];
      int band=motion_band(original.y,icon->bounds.h*units,reverse);
      int lag=motion_band_offset(progress,band);
      GPoint point=GPoint(box.origin.x*units+(int32_t)original.x*box.size.w/icon->bounds.w,
        (box.origin.y+lag)*units+(int32_t)original.y*box.size.h/icon->bounds.h);
      gdraw_command_set_point(s->command,j,point);
    }
    if(s->radius)gdraw_command_set_radius(s->command,(uint32_t)s->radius*box.size.w/icon->bounds.w);
    // Keep the original outline weight at these small sizes.
    gdraw_command_set_stroke_width(s->command,s->stroke);
  }
  // Points already include their sole translation. List drawing avoids clipping
  // them to the original 25px image bounds. Precise paths retain 13.3 units.
  gdraw_command_list_draw(ctx,gdraw_command_image_get_command_list(icon->image));
  return true;
}
#endif
