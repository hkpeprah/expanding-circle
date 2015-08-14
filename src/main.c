#include "storage.h"
#include "timekeeper.h"

#include "utils/logging.h"
#include "utils/fallback.h"
#include "utils/constants.h"
#include "utils/utils.h"

#include <pebble.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  Layer *watchface;
  GColor day_colour;
  GColor night_colour;
  TimeData time;
  union {
    bool is_day;
    uint8_t flags;
  };
} WindowData;

// Private API
/////////////////////////////
static bool prv_in_night_range(uint8_t hours) {
  return (hours < 12);
}

static int32_t prv_hour_angle_from_time(TimeData *timep) {
  int32_t angle = (timep->hours % 12) * (TRIG_MAX_ANGLE / HOURS_PER_HALF_DAY) +
    (timep->minutes * ((TRIG_MAX_ANGLE / MINUTES_PER_HOUR) / HOURS_PER_HALF_DAY));
  return angle % TRIG_MAX_ANGLE;
}

static int32_t prv_minute_angle_from_time(TimeData *timep) {
  int32_t angle = (timep->minutes * (TRIG_MAX_ANGLE / MINUTES_PER_HOUR)) +
    (timep->seconds * ((TRIG_MAX_ANGLE / SECONDS_PER_MINUTE) / MINUTES_PER_HOUR));
  return angle % TRIG_MAX_ANGLE;
}

static void prv_timekeeper_callback(TimeData *timep, void *callback_context) {
  WindowData *data = callback_context;
  data->time = *timep;
  data->is_day = !prv_in_night_range(timep->hours);
  layer_mark_dirty(data->watchface);
}

static void prv_watchface_update_proc(Layer *layer, GContext *ctx) {
  WindowData *data = *((WindowData **)layer_get_data(layer));
  GColor bg_colour, fg_colour;
  if (data->is_day) {
    fg_colour = data->night_colour;
    bg_colour = data->day_colour;
  } else {
    fg_colour = data->day_colour;
    bg_colour = data->night_colour;
  }

  const GRect bounds = layer_get_bounds(layer);
  const GPoint center = grect_center_point(&bounds);
  const uint16_t radius = bounds.size.w / 2;
  const uint16_t inner_radius = (radius * 10000 * (data->time.hours % HOURS_PER_HALF_DAY)) /
    (HOURS_PER_HALF_DAY * 10000) +
    (radius * data->time.minutes * 10000) /
    (HOURS_PER_HALF_DAY * MINUTES_PER_HOUR * 10000);

  // Fill in the background with the background colour.  If this shouldn't be
  // visible, the foreground colour will hide it.
  graphics_context_set_fill_color(ctx, bg_colour);
  graphics_fill_rect(ctx, bounds, 0 /* corner_radius */, GCornerNone);

  // Fill in the inner circle.  This will cover up portions of the background based
  // on the percentage that should be visible.
  graphics_context_set_fill_color(ctx, fg_colour);
  graphics_fill_circle(ctx, center, inner_radius);

  // Spacing to use between any two elements displayed
  const uint16_t padding = 3;
  const uint16_t hour_radius = 4;

  // We want to draw the circle to display the hours.  This is either drawn on the inner circle
  // or the outer circle depending on the percentage of the background that is covered by the
  // inner circle;
  GPoint hour_point = (GPoint){0};
  const bool draw_hours_inside = inner_radius > radius / 2;
  const int32_t hour_angle = prv_hour_angle_from_time(&data->time);
  if (draw_hours_inside) {
    // If the inner circle takes up more than half of the screen, then we want to draw the
    // hour indicator on the inside.
    graphics_context_set_fill_color(ctx, bg_colour);

    unsigned int w = inner_radius - padding - hour_radius;
    GRect centerRect = { GPoint(center.x - w, center.y - w), GSize(w * 2, w * 2) };
    hour_point = gpoint_from_polar(centerRect, GOvalScaleModeFitCircle, hour_angle);
  } else {
    graphics_context_set_fill_color(ctx, fg_colour);

    unsigned int w = radius - padding - hour_radius;
    GRect centerRect = { GPoint(center.x - w, center.y - w), GSize(w * 2, w * 2) };
    hour_point = gpoint_from_polar(centerRect, GOvalScaleModeFitCircle, hour_angle);
  }

  graphics_fill_circle(ctx, hour_point, hour_radius);

  // We want to draw a line to display the minutes.  This is either drawn on the inner circle
  // or the outer circle depending on the percentage of the background that is covered by the
  // inner circle.
  GPoint offset = (GPoint){0};
  GPoint minute_point = (GPoint){0};
  const bool draw_hands_inside = (radius - inner_radius <= radius / 3);
  const int32_t minute_angle = prv_minute_angle_from_time(&data->time);

  // We add padding to offset from the edges of the screen.  We have to add additional padding
  // if the hour hand would intersect with the circle drawn for hte hours.
  int16_t minute_padding = padding * 2;
  if (draw_hands_inside == draw_hours_inside) {
    // Check if the time and minute displays will overlap each other.  If so, add more padding
    // for when we draw the minute hand.
    const int32_t limit = (TRIG_MAX_ANGLE / 360) * (hour_radius * 2);
    if (ABS(hour_angle - minute_angle) <= limit) {
      minute_padding += hour_radius * 2 + padding;
    }
  }

  if (draw_hands_inside) {
    // Since we're drawing the hand in the centre as opposed to the edge, we want to draw another
    // inner circle within the inner circle to represent with an inverse of the inner radius.
    const uint16_t inner_inner_radius = ((radius - inner_radius) / HOURS_PER_HALF_DAY) +
      ((radius - inner_radius) / (HOURS_PER_HALF_DAY * MINUTES_PER_HOUR));
    const uint16_t length = inner_radius - minute_padding - inner_inner_radius - 2 * padding;

    graphics_context_set_stroke_color(ctx, bg_colour);
    graphics_context_set_fill_color(ctx, bg_colour);

    GRect offsetRect = { GPoint(center.x - 2 * padding, center.y - 2 * padding), GSize(4 * padding, 4 * padding) };
    offset = gpoint_from_polar(offsetRect, GOvalScaleModeFitCircle, minute_angle);

    GRect minuteRect = { GPoint(offset.x - length, offset.y - length), GSize(2 * length, 2 * length) };
    minute_point = gpoint_from_polar(minuteRect, GOvalScaleModeFitCircle, minute_angle);

    graphics_fill_circle(ctx, center, inner_inner_radius);
  } else {
    graphics_context_set_stroke_color(ctx, fg_colour);

    const uint16_t length = radius - inner_radius - minute_padding - 2 * padding;

    const unsigned int w = inner_radius + 2 * padding;
    GRect offsetRect = { GPoint(center.x - w, center.y - w), GSize(2 * w, 2 * w) };
    offset = gpoint_from_polar(offsetRect, GOvalScaleModeFitCircle, minute_angle);

    GRect minuteRect = { GPoint(offset.x - length, offset.y - length), GSize(2 * length, 2 * length) };
    minute_point = gpoint_from_polar(minuteRect, GOvalScaleModeFitCircle, minute_angle);
  }

  graphics_context_set_stroke_width(ctx, hour_radius /* pixel stroke width */);
  graphics_draw_line(ctx, offset, minute_point);
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect frame = layer_get_frame(window_layer);
  WindowData *data = malloc(sizeof(WindowData));
  window_set_user_data(window, data);

  memset(data, 0, sizeof(WindowData));
  storage_get_colours(&data->day_colour, &data->night_colour);

  Layer *watchface = layer_create_with_data(frame, sizeof(WindowData **));
  WindowData **watchface_data = layer_get_data(watchface);
  *watchface_data = data;
  data->watchface = watchface;

  layer_set_update_proc(watchface, prv_watchface_update_proc);
  layer_add_child(window_layer, watchface);

  time_keeper_register_callback(prv_timekeeper_callback, data);
}

static void prv_window_unload(Window *window) {
  WindowData *data = window_get_user_data(window);
  if (data) {
    if (data->watchface) {
      layer_destroy(data->watchface);
    }
    memset(data, 0, sizeof(WindowData));
    free(data);
  }
  window_destroy(window);
}

static void prv_init(void) {
  Window *window = window_create();
  window_set_window_handlers(window, (WindowHandlers){
    .load = prv_window_load,
    .unload = prv_window_unload
  });

  time_keeper_init();

  const bool animated = true;
  window_stack_push(window, animated);
}

static void prv_deinit(void) {
  time_keeper_deinit();
}

// App Entrypoint
/////////////////////////////
int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
  return 0;
}
