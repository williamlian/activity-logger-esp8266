#include "blinker.h"
#include <Ticker.h>
#include <Arduino.h>
#include "config.h"
#include "util.h"

Ticker ticker;

bool shoudBlink[BUTTON_COUNT];
bool blinkOn[BUTTON_COUNT];

bool shouldBlinkOnBoard = false;
bool blinkOnOnBoard = false;

void _ticker_blink();

// Sets up all-blinking state and start blinker ticker
void blinkAll() {
  for (int i = 0; i < BUTTON_COUNT; i++) {
    blinkOn[i] = true;
    shoudBlink[i] = true;
  }
  shouldBlinkOnBoard = true;
  ticker.attach(0.1, _ticker_blink);
}

// Sets up blinking state and start blinker ticker
void blink(int button) {
  for (int i = 0; i < BUTTON_COUNT; i++) {
    blinkOn[i] = i == button ? true : false;
    shoudBlink[i] = i == button;
  }
  ticker.attach(0.1, _ticker_blink);
}

void stopBlink() {
  for (int i = 0; i < BUTTON_COUNT; i++) {
    blinkOn[i] = i == false;
    shoudBlink[i] = false;
  }
  shouldBlinkOnBoard = false;
  digitalWrite(STATUS_LED, STATUS_OFF_LEVEL);
  ticker.detach();
}

void blinkOnBoard() {
  shouldBlinkOnBoard = true;
  ticker.attach(0.1, _ticker_blink);
}

void _ticker_blink() {
  for(int i = 0; i < BUTTON_COUNT; i++) {
    if(shoudBlink[i]) {
      digitalWrite(getLedPin(i), blinkOn[i] ? HIGH : LOW);
      blinkOn[i] = !blinkOn[i];
    }
  }
  if(shouldBlinkOnBoard) {
    digitalWrite(STATUS_LED, blinkOnOnBoard ? HIGH : LOW);
    blinkOnOnBoard = !blinkOnOnBoard;
  }
}
