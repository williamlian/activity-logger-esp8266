#include <ESP8266WiFi.h>
#include "logger.h"
#include "wifi.h"
#include "web.h"
#include "config.h"
#include "blinker.h"
#include "util.h"

#define BAUD_RATE 115200
ADC_MODE(ADC_VCC);

/*
 * On task: 85mA, duration: 5s
 * Wifi Off: 25mA
 * Light Sleep: 10mA
 * 
 * Modem Sleep:
 * For 1 minute update:
 * total task time = 60 * 5s = 300s. 300 / 3600 * 85 = 5.4mAh
 * total idle time = 3600 - 300 = 3300s.  3300/3600*25 = 23mAh
 * 
 * 1 day = (5.4 + 23) * 24 = 681 mAh
 * 
 * Light Sleep:
 * For 1m update:
 * total task time 5.4mAh
 * total idel time = 3600 - 300 = 3300s. 3300/3600*10 = 9.2mAh
 * 
 * 1 day = (5.4 + 9.2) * 24 = 350 mAh
 */

/* *********************************************
   Tasks
 * ********************************************/

// task to sync state
void syncState();

// task to receive button event
bool checkPress();
void onClick();

// task to read web response
void onActivityStarted(bool);
void onActivityEnded(bool);
void onActivitySynced(int);

/* *********************************************
   Implementation
 * ********************************************/
ActivityLogger logger("MAIN");
ActivityWifi wifi;
ActivityWebClient webClient;

int busyButton = -1;

void setup() {
  Serial.begin(BAUD_RATE);
  Serial.println();
  logger.log("SDK version: %s \n", ESP.getSdkVersion());
  initLed();
#ifdef SLEEP_MODE_DEEP
  // deep sleep need to re-initialize memory
  logger = ActivityLogger("MAIN");
  wifi = ActivityWifi();
  webClient = ActivityWebClient();
#else
  blinkAll();
  wifi.connectWifi();
#endif
}

void loop() {
#ifdef SLEEP_MODE_AUTO
  loop_autoSleep();
#endif
#ifdef SLEEP_MODE_MODEM
  loop_modemSleep();
#endif
#ifdef SLEEP_MODE_LIGHT
  loop_lightSleep();
#endif
#ifdef SLEEP_MODE_DEEP
  loop_deepSleep();
#endif
}

int loopCounter = 0;

// FIXME: when in sync, button detector is not working
inline void loop_autoSleep() {
  if(!checkPress() && loopCounter == 0) {
    syncState();
    Serial.println();
    Serial.println();
  }
  loopCounter = (loopCounter + 1) % SYNC_MARK_FOR_AUTO_SLEEP;
  delay(LOOP_DURATION_FOR_AUTO_SLEEP);
}

// FIXME: when in sync, button detector is not working
inline void loop_modemSleep() {
  if(!checkPress() && loopCounter == 0) {
    syncState();
    Serial.println();
    Serial.println();
  }
  loopCounter = (loopCounter + 1) % SYNC_MARK;
  wifi.shutDown();
  delay(LOOP_DURATION);
}

// FIXME: interrupt is not working
inline void loop_lightSleep() {
  if(!checkPress()) {
    syncState();
  }
  wifi.lightSleep(SYNC_DURATION, getBtnPin(0));
  Serial.println();
  Serial.println();
}

inline void loop_deepSleep() {
  String resetCause = ESP.getResetReason();
  logger.log("\nReset reason = %s, wiating for WIFI", resetCause.c_str());
  if (resetCause == "External System" || resetCause == "Power on") {
    blinkAll();
    wifi.connectWifi();
    syncState();
  } else if(digitalRead(getBtnPin(0)) == LOW) {
    logger.log("wake up from button");
    blinkAll();
    wifi.connectWifi();
    //onClick(0);
  } else {
    logger.log("wake up from timer");
    blinkOnBoard();
    wifi.connectWifi();
    //syncState();
  }
  wifi.shutDown();
  logger.log("start deep sleep.\n\n");
  Serial.flush();
  ESP.deepSleepInstant((uint32)SYNC_DURATION * 1000, WAKE_NO_RFCAL);
  delay(50);
}

void wakeUpAndConnect(int button = -1) {
  if(button == -1) {
    blinkOnBoard();
  } else {
    blink(button);
  }
  if(!wifi.isConnected()) {
#ifdef SLEEP_MODE_LIGHT
    wifi.wakeUpLightSleep();
#endif 
#ifdef SLEEP_MODE_MODEM
    wifi.wakeUp();
#endif
  }
}

// true if button action activated, otherwise false
bool checkPress() {
  if(busyButton != -1) {
    return true;
  }
  for (int i = 0; i < BUTTON_COUNT; i++) {
    if (digitalRead(getBtnPin(i)) == LOW) {
      logger.log("Button %d Type %d Down, waiting for up...", i, getType(i));
      while (digitalRead(getBtnPin(i)) == LOW) {  // now wait for them to release the pushbutton
        delay(10);
      }
      logger.log("Button %d Type %d Up", i, getType(i));
      onClick(i);
      return true;
    }
  }
  return false;
}

// fetch last active type from server and sync the light state
void syncState() {
  wakeUpAndConnect();
  webClient.getLastActivityType(USER_ID, &onActivitySynced);
  while(webClient.looper()) {
    yield();
  }
}

void onClick(int button) {
  busyButton = button;
  blink(button);
  bool currentOn = isLedOn(button);
  bool ok;
  wakeUpAndConnect(button);
  if(currentOn) {
    ok = webClient.endActivity(USER_ID, getType(button), &onActivityEnded);
  } else {
    ok = webClient.startActivity(USER_ID, getType(button), &onActivityStarted);
  }
  if(!ok) {
    syncState();
  } else {
    while(webClient.looper()) {
      yield();
    }
  }
}

/* *********************************************
   Event handlers
 * ********************************************/
void onActivityStarted(bool ok) {
  stopBlink();
  if(ok) {
    updateLed(busyButton, true);
  } else {
    resetLed(busyButton);
  }
  busyButton = -1;
}

void onActivityEnded(bool ok) {
  stopBlink();
  if(ok) {
    updateLed(busyButton, false);
  } else {
    resetLed(busyButton);
  }
  busyButton = -1;
}

void onActivitySynced(int activeType) {
  stopBlink();
  for(int i = 0; i < BUTTON_COUNT; i++) {
    updateLed(i, getType(i) == activeType);
  }
}
