#include "glance_plan.h"
#include <string.h>

bool glance_plan_pick(const GlancePlan *plan, int32_t now, unsigned limit,
                      unsigned position, GlanceEntry *entry) {
  if (!limit || position>=limit) { return false; }
  unsigned available=0;
  for (unsigned i=0;i<plan->count;i++) {
    if (!plan->entries[i].expires || plan->entries[i].expires>now) { available++; }
  }
  if (position>=available) { return false; }
  if (limit>1 && available>limit && position==limit-1) {
    memset(entry,0,sizeof(*entry));
    strcpy(entry->text,"Open to view upcoming flights");
    return true;
  }
  unsigned index=0;
  for (unsigned i=0;i<plan->count;i++) {
    if (plan->entries[i].expires && plan->entries[i].expires<=now) { continue; }
    if (index++==position) { *entry=plan->entries[i]; return true; }
  }
  return false;
}

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
