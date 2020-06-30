#ifndef ACTIVITY_UTIL
#define ACTIVITY_UTIL
#include "config.h"

/* *********************************************
   Utils
 * ********************************************/

void initLed();

void updateLed(int button, bool turnOn);

void resetLed(int button);

bool isLedOn(int button);

inline int getLedPin(int i) {
  return LED_PIN_MAP[i];
}

inline int getBtnPin(int i) {
  return BTN_PIN_MAP[i];
}

inline int getType(int i) {
  return BTN_TYPE_MAP[i];
}
#endif
