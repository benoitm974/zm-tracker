/*
 * Define all globals from the projects
 *
 */

#pragma once

#include "arduino.h"
#include "secrets.h"

#define CPU_CORE_0 0
#define CPU_CORE_1 1

//#define LOG_TO_BT 1 //1 TO LOG TO BLUETOOTH

#define WIFI_HOME_OFF 0x00
#define WIFI_HOME_ON 0x02

#define HOME_DISTANCE_THRES 0.5f //Distance in Km (double)

#define BT_KEEP_ALIVE_INTERVAL 5000
