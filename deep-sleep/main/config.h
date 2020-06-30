#ifndef ACTIVITY_CONFIG
#define ACTIVITY_CONFIG
#include "pins.h"

#define BAUD_RATE 74880

#define LED_PIN     D1
#define BTN_PIN     D2
#define STATUS_PIN  D8

static const int USER_ID = 0;
static const int ACTIVITY_TYPE = 1;
static const uint32_t SLEEP_MS = 10000;

#endif
