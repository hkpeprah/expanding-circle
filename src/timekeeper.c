#include "timekeeper.h"

#include "utils/constants.h"
#include "utils/logging.h"

#include <pebble.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  TimeKeeperCallback callback;
  void *callback_context;
  AppTimer *timer;
} TimeKeeperData;

static TimeKeeperData *s_time_keeper_data = NULL;

// Private API
/////////////////////////////
#if defined(PBL_DEMO_LOOP)
static void prv_timer_callback(void *context) {
  static TimeData time = {0};
  TimeKeeperData *data = context;
  if (s_time_keeper_data->callback) {
    s_time_keeper_data->callback(&time, s_time_keeper_data->callback_context);
  }

  if (++time.minutes >= MINUTES_PER_HOUR) {
    time.minutes = 0;
    if (++time.hours >= HOURS_PER_DAY) {
      time.hours = 0;
    }
  }

  data->timer = app_timer_register(TIMEOUT_MS, prv_timer_callback, context);
}
#else
static void prv_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if (!s_time_keeper_data) {
    WARN("Tick handler called but no TimeKeeper data");
    return;
  } else if (s_time_keeper_data->callback) {
    TimeData timep = (TimeData){0};
    timep.hours = tick_time->tm_hour;
    timep.minutes = tick_time->tm_min;
    timep.seconds = tick_time->tm_sec;
    s_time_keeper_data->callback(&timep, s_time_keeper_data->callback_context);
  }
}
#endif

// Public API
/////////////////////////////
void time_keeper_init(void) {
  TimeKeeperData *data = malloc(sizeof(TimeKeeperData));
  memset(data, 0, sizeof(TimeKeeperData));
  s_time_keeper_data = data;
#if defined(PBL_DEMO_LOOP)
  data->timer = app_timer_register(TIMEOUT_MS, prv_timer_callback, data);
#else
  tick_timer_service_subscribe(MINUTE_UNIT, prv_tick_handler);
#endif
}

void time_keeper_deinit(void) {
  tick_timer_service_unsubscribe();
  if (s_time_keeper_data) {
    app_timer_cancel(s_time_keeper_data->timer);
    memset(s_time_keeper_data, 0, sizeof(TimeKeeperData));
    free(s_time_keeper_data);
  } else {
    WARN("TimeKeeper deinit called without having init'd");
  }
}

void time_keeper_register_callback(TimeKeeperCallback callback, void *callback_context) {
  if (!s_time_keeper_data) {
    WARN("TimeKeeper called to register callback without being init'd");
    time_keeper_init();
  }
  *s_time_keeper_data = (TimeKeeperData){
    .callback = callback,
    .callback_context = callback_context
  };
#if !defined(PBL_DEMO_LOOP)
  time_t current_time = time(NULL);
  prv_tick_handler(localtime(&current_time), (TimeUnits){0});
#endif
}
