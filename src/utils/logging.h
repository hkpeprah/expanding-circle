#pragma once
#include <pebble.h>

#define __APP_NAME__  "Expanding Circle"
#define __AUTHOR___   "Ford Peprah"

#define DEBUG(fmt, ...)    APP_LOG(APP_LOG_LEVEL_DEBUG, fmt, ## __VA_ARGS__)
#define WARN(fmt, ...)     APP_LOG(APP_LOG_LEVEL_WARNING, fmt, ## __VA_ARGS__)
#define ERROR(fmt, ...)    APP_LOG(APP_LOG_LEVEL_ERROR, fmt, ## __VA_ARGS__)
#define INFO(fmt, ...)     APP_LOG(APP_LOG_LEVEL_INFO, fmt, ## __VA_ARGS__)
