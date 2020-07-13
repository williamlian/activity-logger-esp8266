#ifndef ACTIVITY_CONFIG
#define ACTIVITY_CONFIG
#include "pins.h"

/* *********************************************
   Const & Setting
 * ********************************************/

/* define the sleep mode: AUTO, MODEM, LIGHT, DEEP
   MODEM: 25mA (600mAh/day)
   AUTO:  15~85mA

   ** Console box cannot work in below mode since they need one pin to activate from sleep. **
   LIGHT: 10mA
*/
#define SLEEP_MODE_AUTO
//#define SLEEP_MODE_MODEM

const int USER_ID = 0;
const int BUTTON_COUNT = 1;
const int STATUS_LED = D8;

// Button ID => Pin ID
const int BTN_PIN_MAP[] = {D1, D2, D3, D4, D5};
// LED ID => Pin ID
const int LED_PIN_MAP[] = {D6, D7, D8, D9, D10};
// Button ID => Activity Type ID
const int BTN_TYPE_MAP[] = {2, -1, -1, -1, -1};

// Loop durations, for auto sleep loop duration is longer
const int SYNC_DURATION = 60000;
const int LOOP_DURATION = 100;

const int SYNC_MARK = SYNC_DURATION / LOOP_DURATION;

const int LOOP_DURATION_FOR_AUTO_SLEEP = 350;
const int SYNC_MARK_FOR_AUTO_SLEEP = SYNC_DURATION / LOOP_DURATION_FOR_AUTO_SLEEP;

// LED Pin levels
const int ON_LEVEL = LOW;
const int OFF_LEVEL = HIGH;

// Onboard LED Pin level
const int STATUS_ON_LEVEL = LOW;
const int STATUS_OFF_LEVEL = HIGH;

#endif
