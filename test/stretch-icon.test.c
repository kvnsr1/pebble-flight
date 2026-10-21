#include <assert.h>
#include <stdio.h>
#include "pebble-stub/pebble.h"
struct GDrawCommand {GDrawCommandType type;unsigned count,radius,stroke;GPoint points[4];uint8_t fill;bool closed;};
struct GDrawCommandList {unsigned count;GDrawCommand commands[2];};
struct GDrawCommandImage {GSize size;GDrawCommandList list;};
static int images,allocations,budget=-1,draws;
static void *checked_malloc(size_t n){if(!budget)return NULL;if(budget>0)budget--;void *p=malloc(n);if(p)allocations++;return p;}
static void *checked_calloc(size_t n,size_t s){if(!budget)return NULL;if(budget>0)budget--;void *p=calloc(n,s);if(p)allocations++;return p;}
static void checked_free(void *p){if(p)allocations--;free(p);}
#define malloc checked_malloc
#define calloc checked_calloc
#define free checked_free
#include "../src/c/prototype/stretch_icon.c"
#undef malloc
#undef calloc
#undef free
GSize gdraw_command_image_get_bounds_size(GDrawCommandImage *i){return i->size;}
GDrawCommandList *gdraw_command_image_get_command_list(GDrawCommandImage *i){return &i->list;}
uint32_t gdraw_command_list_get_num_commands(GDrawCommandList *l){return l->count;}
GDrawCommand *gdraw_command_list_get_command(GDrawCommandList *l,uint16_t i){return &l->commands[i];}
uint16_t gdraw_command_get_num_points(GDrawCommand *c){return c->count;}
GDrawCommandType gdraw_command_get_type(GDrawCommand *c){return c->type;}
uint16_t gdraw_command_get_radius(GDrawCommand *c){return c->radius;}
uint8_t gdraw_command_get_stroke_width(GDrawCommand *c){return c->stroke;}
GPoint gdraw_command_get_point(GDrawCommand *c,uint16_t i){return c->points[i];}
void gdraw_command_set_point(GDrawCommand *c,uint16_t i,GPoint p){c->points[i]=p;}
void gdraw_command_set_radius(GDrawCommand *c,uint16_t r){c->radius=r;}
void gdraw_command_set_stroke_width(GDrawCommand *c,uint8_t w){c->stroke=w;}
void gdraw_command_image_destroy(GDrawCommandImage *i){images--;free(i);}
void gdraw_command_list_draw(GContext *ctx,GDrawCommandList *list){(void)ctx;(void)list;draws++;}
static GDrawCommandImage *asset(void){
  GDrawCommandImage *i=calloc(1,sizeof(*i));images++;i->size=GSize(25,25);i->list.count=2;
  i->list.commands[0]=(GDrawCommand){.type=GDrawCommandTypePrecisePath,.count=3,.stroke=1,.fill=0xc3,.closed=true,.points={{-1,0},{200,100},{80,200}}};
  i->list.commands[1]=(GDrawCommand){.type=GDrawCommandTypeCircle,.count=1,.radius=3,.stroke=2,.fill=0xff,.points={{12,12}}};
  return i;
}
int main(void){
  assert(!stretch_icon_create(NULL));stretch_icon_destroy(NULL);
  assert(!stretch_icon_draw(NULL,NULL,GRect(0,0,25,25),500,false));
  for(int failure=0;failure<3;failure++){
    GDrawCommandImage *owned=asset();budget=failure;
    assert(!stretch_icon_create(owned));assert(!images && !allocations);budget=-1;
  }
  GDrawCommandImage *bad=asset();bad->size.h=0;assert(!stretch_icon_create(bad));assert(!images && !allocations);
  bad=asset();bad->list.commands[0].count=2049;assert(!stretch_icon_create(bad));assert(!images && !allocations);
  for(int cycle=0;cycle<30;cycle++){
    GDrawCommandImage *source=asset(),*copy=asset();memcpy(copy,source,sizeof(*copy));
    GDrawCommandImage unchanged=*source;StretchIcon *icon=stretch_icon_create(copy);assert(icon);
    for(int p=0;p<=1000;p+=25)assert(stretch_icon_draw(icon,NULL,GRect(70,60,35,35),p,cycle%2));
    assert(!memcmp(source,&unchanged,sizeof(*source)));
    stretch_icon_draw(icon,NULL,GRect(86,66,25,25),1000,false);
    assert(copy->list.commands[0].points[1].x==86*8+200);
    assert(copy->list.commands[0].points[1].y==66*8+100);
    assert(copy->list.commands[1].points[0].x==98 && copy->list.commands[1].points[0].y==78);
    assert(copy->list.commands[1].radius==3 && copy->list.commands[1].stroke==2);
    assert(copy->list.commands[0].fill==0xc3 && copy->list.commands[0].closed);
    stretch_icon_draw(icon,NULL,GRect(0,0,25,25),0,false);
    assert(!memcmp(copy,source,sizeof(*copy))); // Every point returns exactly; no accumulated drift.
    stretch_icon_destroy(icon);gdraw_command_image_destroy(source);assert(!images && !allocations);
  }
  assert(draws>1000);
  puts("Stretch vectors: precise coordinates, immutable source, exact reset, ownership and allocation failure passed");
}
