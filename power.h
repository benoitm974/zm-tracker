#pragma once

#include "globals.h"
#include <axp20x.h>

#define AXP_INT GPIO_NUM_35
#define AXP_IRQ 0x01

extern TaskHandle_t taskISRHandler;

class Power
{
  public:
    Power();
    void init();
    void powerTest();
    void processIRQ();

    private:
      AXP20X_Class axp;
      static void IRAM_ATTR axpIRQ();
      void setFullPower();
      void setLowPower();
  
};
