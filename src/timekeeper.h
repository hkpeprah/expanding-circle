#pragma once
#include <pebble.h>

typedef struct {
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;
  bool is_24h;
} TimeData;

// Function signature for handling updates to the time
// @param timep Pointer to the TimeData struct holding the time information
// @param callback_context context passed to the callback
typedef void(*TimeKeeperCallback)(TimeData *timep, void *callback_context);

// Called to initialize the TimeKeeper
void time_keeper_init(void);

// Called to deinitialize the TimeKeeper
void time_keeper_deinit(void);

// Called to register a callback for the TimeKeeper
// @param callback The callback to call on time changes
// @param callback_context The context to pass to the callback
void time_keeper_register_callback(TimeKeeperCallback callback, void *callback_context);
