#pragma once
#include "../prototype_config.h"
#include "flight_model.h"
#include "live_store.h"
#if !PEBBLE_MOCK_DATA
void live_data_init(void (*changed)(void));
void live_data_deinit(void);
size_t live_flight_count(void);
const Flight *live_flight_at(size_t index);
const NativeFlight *live_metadata_at(size_t index);
const char *live_global_error(void);
void live_select_flight(size_t index,bool manual);
void live_refresh(void);
void live_aircraft(size_t index);
#endif
