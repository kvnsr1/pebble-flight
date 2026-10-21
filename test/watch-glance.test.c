#include <assert.h>
#include <stdio.h>
#include "pebble-stub/pebble.h"
#include "../src/c/prototype/glance_plan.h"
static bool fail_alloc;
static int outstanding,reloads,added;
static char subtitles[11][151];
static AppGlanceReloadCallback pending;
static void *pending_context;
static void *test_malloc(size_t size){if(fail_alloc)return NULL;void *p=malloc(size);if(p)outstanding++;return p;}
static void test_free(void *p){if(p)outstanding--;free(p);}
static time_t test_time(time_t *value){if(value)*value=100;return 100;}
#define malloc test_malloc
#define free test_free
#define time test_time
#include "../src/c/prototype/watch_glance.c"
#undef malloc
#undef free
#undef time
void app_glance_reload(AppGlanceReloadCallback cb,void *context){reloads++;pending=cb;pending_context=context;}
AppGlanceResult app_glance_add_slice(AppGlanceReloadSession *session,AppGlanceSlice slice){
  (void)session;assert(added<11);snprintf(subtitles[added++],151,"%s",slice.layout.subtitle_template_string);return APP_GLANCE_RESULT_SUCCESS;
}
static void dispatch(size_t limit){added=0;assert(pending);pending(NULL,limit,pending_context);}
int main(void){
  const char *wire="200\tWN1145 JFK > LHR 12:05 AM EDT\n0\tNo upcoming flights\n";
  watch_glance_deinit();watch_glance_update("invalid");assert(!reloads && !outstanding);
  fail_alloc=true;watch_glance_update(wire);assert(!reloads && !outstanding);
  fail_alloc=false;watch_glance_update(wire);assert(reloads==1 && outstanding==1);
  dispatch(8);assert(added==2 && strstr(subtitles[0],"12:05 AM EDT") && !strcmp(subtitles[1],"No upcoming flights"));
  watch_glance_update(wire);assert(reloads==1 && outstanding==1);
  watch_glance_update("broken\n");assert(reloads==1 && outstanding==1);
  dispatch(1);assert(added==1 && strstr(subtitles[0],"WN1145"));
  // Existing memory is reused even under allocation pressure.
  fail_alloc=true;watch_glance_update("0\tSaved flights\n");assert(reloads==2 && outstanding==1);
  dispatch(8);assert(added==1 && !strcmp(subtitles[0],"Saved flights"));
  for(int i=0;i<100;i++){watch_glance_update(i%2?wire:"0\tOpen Jetebble\n");dispatch(8);assert(outstanding==1);}
  watch_glance_deinit();assert(!outstanding);
  dispatch(8);assert(!added); // A late callback cannot access released memory.
  watch_glance_deinit();assert(!outstanding);
  fail_alloc=false;watch_glance_update(wire);assert(outstanding==1);dispatch(8);assert(added==2);
  watch_glance_deinit();assert(!outstanding);
  puts("Launcher schedule: bounded allocation, failure/retry, reuse, delayed callback and cleanup passed");
}
