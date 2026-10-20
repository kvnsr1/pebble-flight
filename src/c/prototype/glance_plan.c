#include "glance_plan.h"
#include <string.h>

bool glance_plan_parse(const char *wire, GlancePlan *plan) {
  memset(plan,0,sizeof(*plan));
  if (!wire || !*wire) { return false; }
  int32_t previous=0;
  while (*wire) {
    if (plan->count==GLANCE_MAX_SLICES) { return false; }
    uint32_t expires=0;
    if (*wire<'0' || *wire>'9') { return false; }
    while (*wire>='0' && *wire<='9') {
      unsigned digit=*wire++-'0';
      if (expires>((uint32_t)INT32_MAX-digit)/10) { return false; }
      expires=expires*10+digit;
    }
    if (*wire++!='\t') { return false; }
    if (expires && (int32_t)expires<=previous) { return false; }
    GlanceEntry *entry=&plan->entries[plan->count];
    entry->expires=(int32_t)expires;
    unsigned length=0;
    while (*wire && *wire!='\n') {
      // Literal subtitles only: phone-supplied data cannot become templates.
      if (length==GLANCE_TEXT_BYTES || *wire=='{' || *wire=='}' || *wire=='\t' || *wire=='\r') { return false; }
      entry->text[length++]=*wire++;
    }
    if (!length || *wire++!='\n' || (!expires && *wire)) { return false; }
    entry->text[length]='\0'; plan->count++;
    previous=(int32_t)expires;
  }
  return previous==0;
}
