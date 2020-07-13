#include <Ticker.h>
#include "blinker.h"
#include "config.h"

Ticker ticker;

bool shouldBlinkLed = false;
bool shouldBlinkStatus = false;
int ledState;

void _ticker_blink() {
  if(shouldBlinkLed) {
    digitalWrite(LED_PIN, digitalRead(LED_PIN) == LOW ? HIGH : LOW);
  }
  if(shouldBlinkStatus) {
    digitalWrite(STATUS_PIN, digitalRead(STATUS_PIN) == LOW ? HIGH : LOW);
  }
}

void blinkStatus() {
  shouldBlinkLed = false;
  shouldBlinkStatus = true;
  ticker.attach(0.1, _ticker_blink);
}

void blinkAll() {
  ledState = digitalRead(LED_PIN);
  shouldBlinkLed = true;
  shouldBlinkStatus = true;
  ticker.attach(0.1, _ticker_blink);
}

void stopBlink() {
  shouldBlinkLed = false;
  shouldBlinkStatus = false;
  ticker.detach();
  digitalWrite(LED_PIN, ledState);
  digitalWrite(STATUS_PIN, HIGH);
}
