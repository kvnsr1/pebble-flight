#include "../prototype_config.h"
#if PEBBLE_UI_PROTOTYPE
#include "navigation.h"

void navigation_move(PrototypeNavigation *nav, int delta, size_t count) {
  nav->note = false;
  if (nav->page != PAGE_LIST) {
    int page = nav->page + delta;
    if (page >= PAGE_OVERVIEW && page <= PAGE_BOOKING) { nav->page = page; }
  } else if (!nav->empty && count) {
    int selected = (int)nav->selected + delta;
    if (selected < 0 || selected >= (int)count) { return; }
    nav->selected = (size_t)selected;
    if (nav->selected < nav->top) { nav->top = nav->selected; }
    if (nav->selected >= nav->top + VISIBLE_FLIGHTS) {
      nav->top = nav->selected - VISIBLE_FLIGHTS + 1;
    }
  }
}
void navigation_select(PrototypeNavigation *nav, size_t count) {
  if (nav->page == PAGE_LIST) {
    if (!nav->empty && nav->selected < count) { nav->page = PAGE_OVERVIEW; }
  } else { nav->note = !nav->note; }
}
bool navigation_back(PrototypeNavigation *nav) {
  if (nav->page == PAGE_LIST) { return true; }
  nav->page = PAGE_LIST;
  nav->note = false;
  return false;
}
void navigation_toggle_empty(PrototypeNavigation *nav) {
  if (nav->page == PAGE_LIST) { nav->empty = !nav->empty; }
}
#endif
