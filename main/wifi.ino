#include <ESP8266WiFi.h>
#include "wifi.h"
#include "logger.h"
#include "secrets.h"

const int CHECK_PERIOD = 5000;

char ssid[] = ACTIVITY_SECRET_SSID;
char pass[] = ACTIVITY_SECRET_PASS;

int status = WL_IDLE_STATUS;

void ActivityWifi::initWifi() {
  pinMode(WIFI_LED, OUTPUT);
  digitalWrite(WIFI_LED, HIGH);
}

bool ActivityWifi::isConnected() {
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(WIFI_LED, LOW);
    return true;
  } else {
    digitalWrite(WIFI_LED, HIGH);
    return false;
  }
}

void ActivityWifi::connectWifi() {
  // attempt to connect to Wifi network:
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(WIFI_LED, HIGH);
    logger.log("Attempting to connect to WPA SSID: %s", ssid);
    status = WiFi.begin(ssid, pass);
  } else {
    digitalWrite(WIFI_LED, LOW);
    logger.log("Connected to: %s", ssid);
  }
}
