#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
typedef struct {int16_t x,y;} GPoint;
typedef struct {int16_t w,h;} GSize;
typedef struct {GPoint origin;GSize size;} GRect;
#define GPoint(x,y) ((GPoint){(x),(y)})
#define GSize(w,h) ((GSize){(w),(h)})
#define GRect(x,y,w,h) ((GRect){{(x),(y)},{(w),(h)}})
typedef struct GContext GContext;
typedef enum {GDrawCommandTypeInvalid,GDrawCommandTypePath,GDrawCommandTypeCircle,GDrawCommandTypePrecisePath} GDrawCommandType;
typedef struct GDrawCommand GDrawCommand;
typedef struct GDrawCommandImage GDrawCommandImage;
typedef struct GDrawCommandList GDrawCommandList;
GSize gdraw_command_image_get_bounds_size(GDrawCommandImage *);
GDrawCommandList *gdraw_command_image_get_command_list(GDrawCommandImage *);
uint32_t gdraw_command_list_get_num_commands(GDrawCommandList *);
GDrawCommand *gdraw_command_list_get_command(GDrawCommandList *,uint16_t);
uint16_t gdraw_command_get_num_points(GDrawCommand *);
GDrawCommandType gdraw_command_get_type(GDrawCommand *);
uint16_t gdraw_command_get_radius(GDrawCommand *);
uint8_t gdraw_command_get_stroke_width(GDrawCommand *);
GPoint gdraw_command_get_point(GDrawCommand *,uint16_t);
void gdraw_command_set_point(GDrawCommand *,uint16_t,GPoint);
void gdraw_command_set_radius(GDrawCommand *,uint16_t);
void gdraw_command_set_stroke_width(GDrawCommand *,uint8_t);
void gdraw_command_image_destroy(GDrawCommandImage *);
void gdraw_command_list_draw(GContext *,GDrawCommandList *);
