#pragma once

#include <TinyGPS++.h>

extern TaskHandle_t taskGPSHandler, TaskWifiHomeHandler;

class tmGPS
{
  public:
    tmGPS();
    void init();
    void gpsTest();
    bool isFixed();
    void readGPS();
    void wifiHome();

  private:
    TinyGPSPlus gps;

};
