#include "../prototype_config.h"
#if PEBBLE_UI_PROTOTYPE
#include "motion_text.h"
static GFont typeface(void){return fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);}
void motion_text_clear(MotionText *text) {
  if(text->scratch)gbitmap_destroy(text->scratch);
  free(text->mask);
  memset(text,0,sizeof(*text));
}
void motion_text_prepare(MotionText *text,const char *value) {
  motion_text_clear(text);snprintf(text->value,sizeof(text->value),"%s",value);
  GSize size=graphics_text_layout_get_content_size(text->value,typeface(),GRect(0,0,140,25),GTextOverflowModeTrailingEllipsis,GTextAlignmentLeft);
  text->width=size.w>140?140:size.w;
  if(PEBBLE_STRETCH_MOTION) {
    text->mask=calloc((140*25+7)/8,1);
    if(text->mask)text->scratch=gbitmap_create_blank(GSize(140,25),GBitmapFormat8Bit);
  }
}
static bool capture(MotionText *text,GContext *ctx) {
  if(!text->scratch)return false;
  GBitmap *frame=graphics_capture_frame_buffer_format(ctx,GBitmapFormat8Bit);
  if(!frame)return false;
  for(unsigned y=0;y<25;y++) {
    GBitmapDataRowInfo row=gbitmap_get_data_row_info(frame,y);
    if(row.min_x>0 || row.max_x<139){graphics_release_frame_buffer(ctx,frame);return false;}
    memcpy(gbitmap_get_data_row_info(text->scratch,y).data,row.data,140);
  }
  graphics_release_frame_buffer(ctx,frame);
  graphics_context_set_fill_color(ctx,GColorBlack);graphics_fill_rect(ctx,GRect(0,0,140,25),0,GCornerNone);
  graphics_context_set_text_color(ctx,GColorWhite);
  graphics_draw_text(ctx,text->value,typeface(),GRect(0,0,140,25),GTextOverflowModeTrailingEllipsis,GTextAlignmentLeft,NULL);
  frame=graphics_capture_frame_buffer_format(ctx,GBitmapFormat8Bit);
  if(frame) {
    for(unsigned y=0;y<25;y++) {
      const uint8_t *row=gbitmap_get_data_row_info(frame,y).data;
      for(unsigned x=0;x<140;x++)if(row[x]==GColorWhite.argb){unsigned bit=y*140+x;text->mask[bit/8]|=1U<<(bit%8);}
    }
    graphics_release_frame_buffer(ctx,frame);text->ready=true;
  }
  // Restore every scratch pixel in the same draw callback: no visible flash.
  graphics_context_set_compositing_mode(ctx,GCompOpAssign);
  graphics_draw_bitmap_in_rect(ctx,text->scratch,GRect(0,0,140,25));
  return text->ready;
}
void motion_text_draw(MotionText *text,GContext *ctx,GPoint origin,int scale) {
  if(scale==1000 || (!text->ready && !capture(text,ctx))) {
    graphics_context_set_text_color(ctx,GColorWhite);
    graphics_draw_text(ctx,text->value,typeface(),GRect(origin.x,origin.y,140,25),GTextOverflowModeTrailingEllipsis,GTextAlignmentLeft,NULL);
    return;
  }
  int width=text->width*scale/1000,height=25*scale/1000;
  if(width<=0 || height<=0)return;
  graphics_context_set_fill_color(ctx,GColorWhite);
  // Whole-glyph scaling only. Contiguous runs keep drawing calls bounded;
  // original system-font pixels are reused, never deformed into vector bands.
  for(int y=0;y<height;y++) {
    int run=-1,sy=y*25/height;
    for(int x=0;x<=width;x++) {
      int sx=x*text->width/width,bit=sy*140+sx;
      bool ink=x<width && (text->mask[bit/8]&(1U<<(bit%8)));
      if(ink && run<0)run=x;
      if(!ink && run>=0){graphics_fill_rect(ctx,GRect(origin.x+run,origin.y+y,x-run,1),0,GCornerNone);run=-1;}
    }
  }
}
#endif
