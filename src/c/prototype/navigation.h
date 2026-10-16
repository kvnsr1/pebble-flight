#pragma once
#include <stdbool.h>
#include <stddef.h>

enum { PAGE_LIST = -1, PAGE_OVERVIEW, PAGE_DEPARTURE, PAGE_ARRIVAL, PAGE_BOOKING };
enum { VISIBLE_FLIGHTS = 3 };
typedef struct {
  size_t selected;
  size_t top;
  int page;
  bool empty;
  bool note;
} PrototypeNavigation;

void navigation_move(PrototypeNavigation *nav, int delta, size_t count);
void navigation_select(PrototypeNavigation *nav, size_t count);
bool navigation_back(PrototypeNavigation *nav); // true means exit from list.
void navigation_toggle_empty(PrototypeNavigation *nav);
