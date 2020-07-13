#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include "wifi.h"
#include "config.h"
#include "blinker.h"
#include "secrets.h"

static const char* lastActivityPath = "/activity/get?type=last&userID=";
static const char* startPath = "/activity/start";
static const char* endPath = "/activity/end";

bool buttonPressed = false;

/* *********************************************
   Main Logic
 * ********************************************/
 
void setup() {
  Serial.begin(BAUD_RATE);
  Serial.println();
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(STATUS_PIN, OUTPUT);

  buttonPressed = digitalRead(D0) == HIGH;
}

void loop() {
  if(buttonPressed) {
    Serial.println("[Interrupt Mode]");
    blinkAll();
  } else {
    Serial.println("[Timer Wakeup Mode]");
    blinkStatus();
  }
  connectWifi();

  // ------- WEB ---------
  BearSSL::WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  String baseURL = String("https://") + ACTIVITY_SECRET_SERVER;

  if(buttonPressed) {
    if(http.begin(client, baseURL + lastActivityPath + USER_ID)) {
      http.addHeader("Content-Type", "application/json");
      String post = String("{\"UserID\":") + USER_ID + ",\"TypeID\":" + ACTIVITY_TYPE + "}";
      if(http.POST(post) == HTTP_CODE_OK) {
        Serial.println("activity update scucessful");
      } else {
        Serial.println("activity update failed");
      }
      http.end();
    } else {
      Serial.println("failed connect to server");
    }
  } else {
    if(http.begin(client, baseURL + lastActivityPath + USER_ID)) {
      if(http.GET() == HTTP_CODE_OK) {
        Serial.println("sync scucessful");
        String payload = http.getString();
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, payload);
        if(error) {
          Serial.printf("error parsing JSON: %s\n", error);
        } else {
          int activityType = (int)doc["Type"]["ID"];
          if(!(doc["EndedAt"] == nullptr)) {
            activityType = -1;
          }
          Serial.printf("last type: %d\n", activityType);
        }
      } else {
        Serial.println("sync failed");
      }
      http.end();
    } else {
      Serial.println("failed connect to server");
    } 
  }
  
  // ------- END WEB -------

  stopBlink();
  startSleep(SLEEP_MS);
}
