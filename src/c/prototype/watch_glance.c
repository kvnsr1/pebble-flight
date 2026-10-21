#include "../prototype_config.h"
#if PEBBLE_UI_PROTOTYPE && !PEBBLE_MOCK_DATA
#include <pebble.h>
#include "glance_plan.h"
#include "watch_glance.h"

static GlancePlan s_plan;
static void reload(AppGlanceReloadSession *session, size_t limit, void *context) {
  time_t now=time(NULL);
  for (unsigned i=0;i<limit;i++) {
    GlanceEntry entry;
    if (!glance_plan_pick(&s_plan,now,limit,i,&entry)) { break; }
    AppGlanceSlice slice={.layout={.icon=APP_GLANCE_SLICE_DEFAULT_ICON,
      .subtitle_template_string=entry.text},.expiration_time=entry.expires};
    AppGlanceResult result=app_glance_add_slice(session,slice);
    if (result!=APP_GLANCE_RESULT_SUCCESS) {
      APP_LOG(APP_LOG_LEVEL_WARNING,"Launcher slice rejected: %d",result);
      break;
    }
  }
}

void watch_glance_update(const char *wire) {
  GlancePlan next;
  if (!glance_plan_parse(wire,&next)) { return; }
  if (!memcmp(&next,&s_plan,sizeof(next))) { return; }
  s_plan=next;
  app_glance_reload(reload,NULL);
}
#endif
