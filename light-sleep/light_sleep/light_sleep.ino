#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <coredecls.h>
#include <include/WiFiState.h>
#include "wifi.h"
#include "config.h"
#include "blinker.h"
#include "secrets.h"
extern "C" {
  #include "gpio.h"
  #include "lwip/apps/sntp.h"
}

static const char* lastActivityPath = "/activity/get?type=last&userID=";
static const char* startPath = "/activity/start";
static const char* endPath = "/activity/end";

bool buttonPressed = false;
static WiFiState* lightSleepState = (WiFiState*)RTC_USER_MEM;

/* *********************************************
   Main Logic
 * ********************************************/
 
void setup() {
  Serial.begin(BAUD_RATE);
  Serial.println();
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(STATUS_PIN, OUTPUT);

  digitalWrite(LED_PIN, HIGH);
  digitalWrite(STATUS_PIN, HIGH);
}

void loop() {
  if(buttonPressed) {
    //blinkAll();
    Serial.println("[Update Mode]");
  } else {
    //blinkStatus();
    Serial.println("[Sync Mode]");
  }
  wakeUp();
  runWebTask();
  buttonPressed = false;
  //stopBlink();
  saveState();
  sleep();
}

void wakeupCallback() {
  wifi_fpm_close();
  buttonPressed = digitalRead(BTN_PIN) == LOW;
  Serial.println(F("Sleep Interrupted"));
}

void saveState() {
  bool persistent = WiFi.getPersistent();
  WiFiMode_t before_off_mode = WiFi.getMode();
  bool ret = wifi_get_ip_info(STATION_IF, &lightSleepState->state.ip);
  if (!ret) {
      return;
  }
  memset(lightSleepState->state.fwconfig.bssid, 0xff, 6);
  ret = wifi_station_get_config(&lightSleepState->state.fwconfig);
  if (!ret) {
      return;
  }
  lightSleepState->state.channel = wifi_get_channel();
  lightSleepState->state.persistent = persistent;
  lightSleepState->state.mode = before_off_mode;
  uint8_t i = 0;
  for (auto& ntp: lightSleepState->state.ntp) {
    ntp = *sntp_getserver(i++);
  }
  i = 0;
  for (auto& dns: lightSleepState->state.dns)
      dns = WiFi.dnsIP(i++);
  lightSleepState->crc = crc32(&lightSleepState->state, sizeof(lightSleepState->state));
}

void sleep() {
  WiFi.mode(WIFI_OFF);
  // stop (but don't disable) the 4 OS timers
  extern os_timer_t *timer_list;
  timer_list = nullptr;  
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
  gpio_pin_wakeup_enable(BTN_PIN, GPIO_PIN_INTR_LOLEVEL); 
  wifi_fpm_set_wakeup_cb(wakeupCallback);
  Serial.println("start sleep\n\n");
  Serial.flush();
  wifi_fpm_open();
  wifi_fpm_do_sleep(SLEEP_MS * 1000);
  delay(SLEEP_MS + 1);
}

void wakeUp() {
  if(WiFi.mode(WIFI_RESUME, lightSleepState)) {
    uint32_t wifiBegin = millis();
    Serial.print("resume Wifi...");
    while (!WiFi.localIP() || WiFi.status() != WL_CONNECTED) {
      yield();
    }
    Serial.printf("done, time = %d\n", millis() - wifiBegin);
  } else {
    Serial.println("failed, reconnect...");
    connectWifi();
  }
}

void runWebTask() {
  BearSSL::WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  String baseURL = String("https://") + ACTIVITY_SECRET_SERVER;

  if(buttonPressed) {
    String url = baseURL;
    if(digitalRead(LED_PIN) == HIGH) {
      url += startPath;
    } else {
      url += endPath;
    }
    if(http.begin(client, url)) {
      http.addHeader("Content-Type", "application/json");
      String post = String("{\"UserID\":") + USER_ID + ",\"TypeID\":" + ACTIVITY_TYPE + "}";
      if(http.POST(post) == HTTP_CODE_OK) {
        Serial.println("activity update scucessful");
        digitalWrite(LED_PIN, digitalRead(LED_PIN) == HIGH ? LOW : HIGH);
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
          if(activityType == ACTIVITY_TYPE) {
            digitalWrite(LED_PIN, LOW);
          } else {
            digitalWrite(LED_PIN, HIGH);
          }
        }
      } else {
        Serial.println("sync failed");
      }
      http.end();
    } else {
      Serial.println("failed connect to server");
    } 
  }
}
