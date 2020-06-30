#ifndef ACTIVITY_WIFI_H
#define ACTIVITY_WIFI_H
#include <TaskScheduler.h>
#include "pins.h"

class ActivityWifi {
    public:
    void initWifi();
    void connectWifi();
    bool isConnected();

    private:
    const int WIFI_LED = D0;
    ActivityLogger logger = ActivityLogger("WIFI");
};

#endif
