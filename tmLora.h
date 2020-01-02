#pragma once

#include "globals.h"

extern TaskHandle_t TaskLoraHandler;

class tmLora
{
  public:
    tmLora();
    void init();
    void loraSend();

  private:
    void do_send(osjob_t* j);
};
