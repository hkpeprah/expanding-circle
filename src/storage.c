#include "storage.h"

#include "utils/constants.h"
#include "utils/fallback.h"

#include <pebble.h>

#define STORAGE_DAY_KEY (42)
#define STORAGE_NIGHT_KEY (1337)

void storage_set_colours(GColor day_colour, GColor night_colour) {
  persist_write_int(STORAGE_DAY_KEY, day_colour.argb);
  persist_write_int(STORAGE_NIGHT_KEY, night_colour.argb);
}

void storage_get_colours(GColor *day_colour, GColor *night_colour) {
  GColor day = DEFAULT_DAY_COLOUR;
  GColor night = DEFAULT_NIGHT_COLOUR;
  if (persist_exists(STORAGE_DAY_KEY)) {
    day.argb = persist_read_int(STORAGE_DAY_KEY);
  }

  if (persist_exists(STORAGE_NIGHT_KEY)) {
    night.argb = persist_read_int(STORAGE_NIGHT_KEY);
  }

  if (day_colour) {
    *day_colour = COLOR_FALLBACK(day, GColorWhite);
  }

  if (night_colour) {
    *night_colour = COLOR_FALLBACK(night, GColorBlack);
  }
}

void storage_reset(void) {
  persist_delete(STORAGE_DAY_KEY);
  persist_delete(STORAGE_NIGHT_KEY);
}
