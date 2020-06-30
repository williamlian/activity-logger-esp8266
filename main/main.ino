#include <TaskScheduler.h>
#include <Ticker.h>
#include "logger.h"
#include "wifi.h"
#include "web.h"

#define BAUD_RATE 115200
#define BLINK -1

/* *********************************************
   Const & Setting
 * ********************************************/
const int USER_ID = 0;
const int BUTTON_COUNT = 1;

// debound ms for click detection
const int DEBOUNCE = 50;

// Button ID => Pin ID
const int BTN_PIN_MAP[] = {D1, D2, D3, D4, D5};
// LED ID => Pin ID
const int LED_PIN_MAP[] = {D6, D7, D8, D9, D10};
// Button ID => Activity Type ID
const int BTN_TYPE_MAP[] = {2, -1, -1, -1, -1};

/* *********************************************
   Tasks
 * ********************************************/
Scheduler scheduler;

// task to blink 
bool workerBlink();
// Task taskBlink(100 * TASK_MILLISECOND, -1, &workerBlink, &scheduler, true);

// task to sync state
void syncState();
Task taskSyncState(5000 * TASK_MILLISECOND, -1, &syncState, &scheduler, true);

// task to receive button event
void workerButton();
void onClick();
Task taskButton(50 * TASK_MILLISECOND, -1, &workerButton, &scheduler, false);

// task to read web response
void workerWeb();
void onActivityStarted(bool);
void onActivityEnded(bool);
void onActivitySynced(int);
Task taskWeb(100 * TASK_MILLISECOND, -1, &workerWeb, &scheduler, false);

/* *********************************************
   Implementation
 * ********************************************/
ActivityLogger logger("MAIN");
ActivityWifi wifi;
ActivityWebClient webClient;

int ledState[BUTTON_COUNT];
bool pressed[BUTTON_COUNT];
int blinkState[BUTTON_COUNT];

Ticker blinker;

void setup() {
  Serial.begin(BAUD_RATE);
  Serial.println();
  for (int i = 0; i < BUTTON_COUNT; i++) {
    pinMode(getBtnPin(i), INPUT_PULLUP);
    pinMode(getLedPin(i), OUTPUT);
    ledState[i] = BLINK;
    blinkState[i] = LOW;
    pressed[i] = false;
  }
  blinker.attach_ms(100, workerBlink);
  wifi.initWifi();
  logger.log("Initalized");
}

void loop() {
  scheduler.execute();
}

bool workerBlink() {
  for(int i = 0; i < BUTTON_COUNT; i++) {
    if(ledState[i] == BLINK) {
      blinkState[i] = blinkState[i] == HIGH ? LOW : HIGH;
      digitalWrite(getLedPin(i), blinkState[i]);
    }
  }
}

void workerButton() {
  for (int i = 0; i < BUTTON_COUNT; i++) {
    if(ledState[i] == BLINK) {
      // skip busy buttons
      continue;
    }
    if (digitalRead(getBtnPin(i)) == LOW) {
      if (!pressed[i]) {
        logger.log("Button %d Type %d Down", i, getType(i));
        pressed[i] = true;
      }
    } else if (pressed[i]) {
      pressed[i] = false;
      logger.log("Button %d Type %d Up", i, getType(i));
      onClick(i);
    }
  }
}

void onClick(int button) {
  int currentState = ledState[button];
  blink(button, HIGH);
  bool ok;
  if(currentState == HIGH) {
    ok = webClient.endActivity(USER_ID, getType(button), &onActivityStarted);
  } else {
    ok = webClient.startActivity(USER_ID, getType(button), &onActivityEnded);
  }
  if(!ok) {
    taskSyncState.forceNextIteration();
  }
}

void onActivityStarted(bool ok) {
  taskSyncState.forceNextIteration();
}

void onActivityEnded(bool ok) {
  taskSyncState.forceNextIteration();
}

void onActivitySynced(int activeType) {
  for(int i = 0; i < BUTTON_COUNT; i++) {
    ledState[i] = (getType(i) == activeType) ? HIGH : LOW;
    updateLed(i);
  }
  logger.log("state sync'ed. active type = %d", activeType);
}

void workerWeb() {
  webClient.looper();
  yield();
}

// fetch last active type from server and sync the light state
void syncState() {
  if(!wifi.isConnected()) {
    taskButton.disable();
    taskWeb.disable();
    blinkAll();
    wifi.connectWifi();
    return;
  }
  taskButton.enable();
  taskWeb.enable();
  webClient.getLastActivityType(USER_ID, &onActivitySynced);
  yield();
}

/* *********************************************
   Utils
 * ********************************************/

void updateLed(int button) {
  digitalWrite(getLedPin(button), ledState[button]);
}

void blinkAll() {
  for (int i = 0; i < BUTTON_COUNT; i++) {
    blinkState[i] = LOW;
    ledState[i] = BLINK;
  }
}

void blink(int button, int initalState) {
  blinkState[button] = initalState;
  ledState[button] = BLINK;
}

int getLedPin(int i) {
  return LED_PIN_MAP[i];
}

int getBtnPin(int i) {
  return BTN_PIN_MAP[i];
}

int getType(int i) {
  return BTN_TYPE_MAP[i];
}
