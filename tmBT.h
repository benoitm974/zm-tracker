#pragma once
#include "globals.h"
#include "BluetoothSerial.h"

class tmBT
{
  public:
    tmBT();
    void init();
    void connect();
    void keepAlive();

  private:
    BluetoothSerial SerialBT;
    bool connected = false;

};
