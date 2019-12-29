#pragma once

#include "globals.h"

#include <WiFi.h>
#include <WiFiMulti.h>

extern TaskHandle_t TaskWifiHomeHandler;

class tmWifi
{
  public:
    tmWifi();
    void init();
    void homeConnect();
    void homeDisconnect();
    void setClock();

  private:

};
