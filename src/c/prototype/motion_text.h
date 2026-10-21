#pragma once
#include <pebble.h>
enum { MOTION_TEXT_WIDTH=140,MOTION_TEXT_HEIGHT=25 };
typedef struct {
  char value[13];
  int width;
  bool ready;
  GBitmap *scratch;
  uint8_t *mask;
} MotionText;
void motion_text_prepare(MotionText *text,const char *value);
void motion_text_clear(MotionText *text);
void motion_text_draw(MotionText *text,GContext *ctx,GPoint origin,int scale);
