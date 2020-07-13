#ifndef ACTIVITY_WIFI_H
#define ACTIVITY_WIFI_H
#include "logger.h"

class ActivityWifi {
    public:
    void connectWifi();
    bool isConnected();

    void shutDown();
    void wakeUp();

    private:
    ActivityLogger logger = ActivityLogger("WIFI");
};

#endif
