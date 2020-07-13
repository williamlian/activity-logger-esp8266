#include <ESP8266WiFi.h>
#include <coredecls.h>         // crc32()
#include <include/WiFiState.h> // WiFiState structure details
#include "wifi.h"
#include "logger.h"
#include "secrets.h"
extern "C" {
  #include "gpio.h"
  #include "lwip/apps/sntp.h"
}

char ssid[] = ACTIVITY_SECRET_SSID;
char pass[] = ACTIVITY_SECRET_PASS;

int status = WL_IDLE_STATUS;

// This structure is stored in RTC memory to save the WiFi state and reset count (number of Deep Sleeps),
// and it reconnects twice as fast as the first connection; it's used several places in this demo
struct nv_s {
  WiFiState wss; // core's WiFi save state
};

static nv_s* nv = (nv_s*)RTC_USER_MEM; // user RTC RAM area

bool ActivityWifi::isConnected() {
  return WiFi.localIP() && WiFi.status() == WL_CONNECTED;
}

void ActivityWifi::shutDown() {
  if(isConnected()) {
    logger.log("shutting down WIFI");
    WiFi.mode(WIFI_SHUTDOWN, &nv->wss);
    Serial.flush();
  }
}

void ActivityWifi::wakeUp() {
  if(!isConnected()) {
    uint32_t wifiBegin = millis();
    logger.log("waking up WIFI");
    WiFi.forceSleepWake();
    while (!WiFi.localIP() || WiFi.status() != WL_CONNECTED) {
      yield();
    }
    logger.log("waked up WIFI, time = %d", millis() - wifiBegin);
  }
}
 
void ActivityWifi::connectWifi() {
  uint32_t wifiBegin = millis();
  if (WiFi.shutdownValidCRC(&nv->wss)) {  // if we have a valid WiFi saved state
    logger.log("Resuming Wifi");
  }
  if (!(WiFi.mode(WIFI_RESUME, &nv->wss))) {  // couldn't resume, or no valid saved WiFi state yet
    /* Explicitly set the ESP8266 as a WiFi-client (STAtion mode), otherwise by default it
      would try to act as both a client and an access-point and could cause network issues
      with other WiFi devices on your network. */
    WiFi.persistent(false);  // don't store the connection each time to save wear on the flash
    WiFi.mode(WIFI_STA);
    WiFi.setOutputPower(10);  // reduce RF output power, increase if it won't connect
    
    //WiFi.config(staticIP, gateway, subnet);  // if using static IP, enter parameters at the top
    WiFi.begin(ssid, pass);
    logger.log("Attempting to connect to: %s", ssid);
  }
  while (!WiFi.localIP() || WiFi.status() != WL_CONNECTED) {
    yield();
  }
  logger.log("Connected to: %s with IP %s", ssid, WiFi.localIP().toString().c_str());
  WiFi.setAutoReconnect(true);
  logger.log("WiFi connect time = %d", millis() - wifiBegin);
}
