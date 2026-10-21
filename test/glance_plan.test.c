#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/c/prototype/glance_plan.h"

int main(void) {
  GlancePlan plan;
  assert(glance_plan_parse("1792440000\tWN1145 · BUR>OAK\n1792450000\tOpen for flight status\n0\tNo upcoming flights\n",&plan));
  assert(plan.count==3 && plan.entries[0].expires==1792440000);
  assert(!strcmp(plan.entries[0].text,"WN1145 · BUR>OAK"));
  const char *bad[]={"","0\t\n","0\tNo newline","1\tFirst\n1\tEqual\n0\tEnd\n",
    "10\tFirst\n9\tBackwards\n0\tEnd\n","0\tEnd\n10\tAfter end\n","1\tNo permanent fallback\n",
    "2147483648\tOverflow\n0\tEnd\n","999999999999\tOverflow\n0\tEnd\n","-1\tNegative\n",
    "0\tBad\ttab\n","0\tBad\rreturn\n","0\t{template}\n"};
  for (unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);i++) { assert(!glance_plan_parse(bad[i],&plan)); }
  char long_wire[200]; memset(long_wire,'A',sizeof(long_wire));
  memcpy(long_wire,"0\t",2); long_wire[152]='\n';long_wire[153]='\0';
  assert(glance_plan_parse(long_wire,&plan)); // exactly 150 subtitle bytes
  long_wire[152]='A';long_wire[153]='\n';long_wire[154]='\0';
  assert(!glance_plan_parse(long_wire,&plan));
  assert(!glance_plan_parse("1\ta\n2\tb\n3\tc\n4\td\n5\te\n6\tf\n7\tg\n8\th\n9\ti\n10\tj\n11\tk\n0\tl\n",&plan));
  assert(glance_plan_parse("1\ta\n2\tb\n3\tc\n4\td\n5\te\n6\tf\n0\tg\n",&plan));
  assert(glance_plan_parse("1\ta\n2\tb\n3\tc\n4\td\n5\te\n6\tf\n7\tg\n8\th\n9\ti\n10\tj\n0\tNo upcoming flights\n",&plan));
  GlanceEntry entry;
  assert(glance_plan_pick(&plan,0,11,10,&entry) && entry.expires==0);
  assert(!strcmp(entry.text,"No upcoming flights"));
  assert(glance_plan_pick(&plan,0,8,7,&entry) && entry.expires==0);
  assert(!strcmp(entry.text,"Open to view upcoming flights"));
  assert(glance_plan_pick(&plan,0,1,0,&entry) && entry.expires==1);
  assert(glance_plan_pick(&plan,5,8,0,&entry) && entry.expires==6);
  assert(glance_plan_pick(&plan,5,8,5,&entry) && entry.expires==0);
  assert(!strcmp(entry.text,"No upcoming flights"));
  assert(!glance_plan_pick(&plan,0,0,0,&entry));
  assert(!glance_plan_pick(&plan,0,8,8,&entry));
  puts("Native launcher wire parser bounds and malformed-plan rejection passed");
}
