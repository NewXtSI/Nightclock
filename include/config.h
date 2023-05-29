#ifndef INCLUDE_CONFIG_H__
#define INCLUDE_CONFIG_H__

#include "userconfig.h"

#ifndef WIFI_SSID
#  pragma error "Please define WIFI_SSID in userconfig.h"
#endif

#ifndef WIFI_PSK
#  pragma error "Please define WIFI_PSK in userconfig.h"
#endif

#define TASK_UI_READY               (1 << 0)

static const uint16_t screenWidth = 480;
static const uint16_t screenHeight = 320;

#endif  // INCLUDE_CONFIG_H__
