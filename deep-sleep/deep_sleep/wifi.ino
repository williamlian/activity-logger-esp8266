#include <include/WiFiState.h>
#include "wifi.h"
#include "secrets.h"

static WiFiState* wifiState = (WiFiState*)RTC_USER_MEM;

void connectWifi() {
  uint32_t wifiBegin = millis();
  if (!(WiFi.mode(WIFI_RESUME, wifiState))) {
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.setOutputPower(10);
    WiFi.begin(ACTIVITY_SECRET_SSID, ACTIVITY_SECRET_PASS);
    Serial.print("Starting new Wifi...");
  } else {
    Serial.print("Resuming Wifi...");
  }

  while (!WiFi.localIP() || WiFi.status() != WL_CONNECTED) {
    yield();
  }
  Serial.printf("connected in %d ms.\n", millis() - wifiBegin);
  WiFi.setAutoReconnect(true);
}

void startSleep(const uint32_t ms) {
  WiFi.mode(WIFI_SHUTDOWN, wifiState);
  Serial.println("Wifi shut down, start deep sleep.\n\n");
  Serial.flush();
  ESP.deepSleepInstant((uint32_t)ms * 1000, WAKE_NO_RFCAL);
}
