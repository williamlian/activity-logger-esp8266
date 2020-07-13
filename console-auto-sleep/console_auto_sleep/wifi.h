#ifndef ACTIVITY_WIFI_H
#define ACTIVITY_WIFI_H
#include "logger.h"

class ActivityWifi {
    public:
    void connectWifi(int wakeUpPin = -1, bool persist = false);
    bool isConnected();

    void shutDown();
    void wakeUp();

    void wakeUpLightSleep();

    void lightSleep(const uint32 sleepMs, const int interruptPin);

    private:
    ActivityLogger logger = ActivityLogger("WIFI");
};

#endif
