#pragma once
#include <pebble.h>

// One-shot launch overlay. The app continues receiving data underneath it.
void startup_show(Layer *root);
bool startup_dismiss(void);
void startup_destroy(void);
