#pragma once

#include <pebble.h>

// Sets the colours for the night and day of the watchface.
// @param day_colour The day colour to set
// @param night_colour The night colour to set
void storage_set_colours(GColor day_colour, GColor night_colour);

// Gets the colours for the night and day of the watchface.
// @param day_colour Pointer to store the day colour
// @param night_colour Pointer to store the night colour
void storage_get_colours(GColor *day_colour, GColor *night_colour);

// Resets the storage values.
void storage_reset(void);
