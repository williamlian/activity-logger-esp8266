#include <Arduino.h>
#include "util.h"

bool ledState[BUTTON_COUNT];

void initLed() {
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, STATUS_OFF_LEVEL);
  for (int i = 0; i < BUTTON_COUNT; i++) {
    pinMode(getBtnPin(i), INPUT_PULLUP);
    pinMode(getLedPin(i), OUTPUT);
    ledState[i] = false;
    digitalWrite(getLedPin(i), OFF_LEVEL);
  }
}

bool isLedOn(int button) {
  return ledState[button];
}

void updateLed(int button, bool turnOn) {
  ledState[button] = turnOn;
  digitalWrite(getLedPin(button), turnOn ? ON_LEVEL : OFF_LEVEL);
}

void resetLed(int button) {
  digitalWrite(getLedPin(button), ledState[button] ? ON_LEVEL : OFF_LEVEL);
}
