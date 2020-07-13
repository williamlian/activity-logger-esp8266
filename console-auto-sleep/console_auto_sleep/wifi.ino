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

WiFiState lightSleepState;
void saveState() {
  bool persistent = WiFi.getPersistent();
  WiFiMode_t before_off_mode = WiFi.getMode();
  bool ret = wifi_get_ip_info(STATION_IF, &lightSleepState.state.ip);
  if (!ret) {
      return;
  }
  memset(lightSleepState.state.fwconfig.bssid, 0xff, 6);
  ret = wifi_station_get_config(&lightSleepState.state.fwconfig);
  if (!ret) {
      return;
  }
  lightSleepState.state.channel = wifi_get_channel();
  lightSleepState.state.persistent = persistent;
  lightSleepState.state.mode = before_off_mode;
  uint8_t i = 0;
  for (auto& ntp: lightSleepState.state.ntp) {
    ntp = *sntp_getserver(i++);
  }
  i = 0;
  for (auto& dns: lightSleepState.state.dns)
      dns = WiFi.dnsIP(i++);
  lightSleepState.crc = crc32(&lightSleepState.state, sizeof(lightSleepState.state));
}

// https://www.espressif.com/sites/default/files/documentation/2c-esp8266_non_os_sdk_api_reference_cn.pdf page 83
void ActivityWifi::lightSleep(const uint32 sleepMs, const int interruptPin) {
  saveState();
  
  wifi_station_disconnect();
  wifi_set_opmode(NULL_MODE);              //  set WiFi  mode  to  null  mode.

  // stop (but don't disable) the 4 OS timers
  extern os_timer_t *timer_list;
  timer_list = nullptr;
  
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
  gpio_pin_wakeup_enable(GPIO_ID_PIN(interruptPin), GPIO_PIN_INTR_LOLEVEL); 
  //wifi_enable_gpio_wakeup(interruptPin, GPIO_PIN_INTR_LOLEVEL);
  wifi_fpm_set_wakeup_cb(wakeupCallback); // set wakeup callback
  wifi_fpm_open();
  // the callback is optional, but without it the modem will wake in 10 seconds then delay(10 seconds)
  // with the callback the sleep time is only 10 seconds total, no extra delay() afterward
  wifi_fpm_do_sleep(sleepMs * (uint32)1000);
  // delay needs to be 1 mS longer than sleep or it only goes into Modem Sleep
  logger.log("CPU off for %d ms, or trigger LOW to pin %d", sleepMs, interruptPin);
  Serial.flush();
  // some delay to let the board complete OS task and start idling
  delay(sleepMs / 2);
}

void wakeupCallback() {
  wifi_fpm_close();         // disable force sleep function
  Serial.println("Sleep Interrupted");
}

void ActivityWifi::wakeUpLightSleep() {
  if(WiFi.mode(WIFI_RESUME, &lightSleepState)) {
    uint32_t wifiBegin = millis();
    logger.log("start resume...");
    while (!WiFi.localIP() || WiFi.status() != WL_CONNECTED) {
      yield();
    }
    logger.log("waked up WIFI, time = %d", millis() - wifiBegin);
  } else {
    logger.log("resume failed, reconnect...");
    connectWifi();
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
 
void ActivityWifi::connectWifi(int wakeUpPin, bool persist) {
  uint32_t wifiBegin = millis();
//  if ((crc32((uint8_t*) &nv->wss + 1, sizeof(nv->wss)) && !WiFi.shutdownValidCRC(&nv->wss))) {
//    // restore wss backip
//    memcpy((uint32_t*) &nv->wss, (uint32_t*) &nv->wss + 1, sizeof(nv->wss));
//  }
  if (WiFi.shutdownValidCRC(&nv->wss)) {  // if we have a valid WiFi saved state
//    memcpy((uint32_t*) &nv->wss + 1, (uint32_t*) &nv->wss, sizeof(nv->wss)); // save a copy of it
    logger.log("Resuming Wifi");
  }
  if (!(WiFi.mode(WIFI_RESUME, &nv->wss))) {  // couldn't resume, or no valid saved WiFi state yet
    /* Explicitly set the ESP8266 as a WiFi-client (STAtion mode), otherwise by default it
      would try to act as both a client and an access-point and could cause network issues
      with other WiFi devices on your network. */
    WiFi.persistent(persist);  // don't store the connection each time to save wear on the flash
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
  
  // Auto light sleep mode
  if(wakeUpPin != -1) {
    logger.log("Operate in auto light sleep mode.", wakeUpPin);
    WiFi.setSleepMode(WIFI_LIGHT_SLEEP);
  }
  WiFi.setAutoReconnect(true);
  logger.log("WiFi connect time = %d", millis() - wifiBegin);
}
