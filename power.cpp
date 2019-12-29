#include "power.h"

static const char TAG[] = __FILE__;


Power::Power() {
  ESP_LOGD(TAG, "Power constructor");
}

void Power::init() {
  // I2C AXP current controller
  Wire.begin(21, 22);
  if (!axp.begin(Wire, AXP192_SLAVE_ADDRESS)) {

    //GENERAL settings for AXP
    ESP_LOGD(TAG, "AXP192 Begin PASS");
    axp.setTimeOutShutdown(false); //no auto shutdown
    axp.setTSmode(AXP_TS_PIN_MODE_DISABLE); // TS pin mode off to save power

    axp.setChargingTargetVoltage(AXP202_TARGET_VOL_4_2V); // Set charging Voltage for 18650 Li-Ion

    //IRQ setup
    pinMode(AXP_INT, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(AXP_INT), Power::axpIRQ, FALLING);
    axp.enableIRQ(AXP202_VBUS_REMOVED_IRQ | AXP202_VBUS_CONNECT_IRQ | AXP202_PEK_SHORTPRESS_IRQ, 1);
    axp.clearIRQ();
    ESP_LOGI(TAG, "AXP192 IRQ init done");

    ESP_LOGD(TAG, "AXP192 Init done.");

  } else {

    ESP_LOGD(TAG, "AXP192 Begin FAIL");

  }
}

void IRAM_ATTR Power::axpIRQ() {
  ESP_LOGD(TAG, "Interupt CALLED!");
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  xTaskNotifyFromISR(taskISRHandler, AXP_IRQ, eSetBits,
                     &xHigherPriorityTaskWoken);

  if (xHigherPriorityTaskWoken)
    portYIELD_FROM_ISR();
}

void Power::processIRQ() {
  axp.readIRQ();

  if (axp.isVbusPlugInIRQ()) {
    ESP_LOGI(TAG, "USB plugged");
    setFullPower();
  }
  if (axp.isVbusRemoveIRQ()) {
    ESP_LOGI(TAG, "USB un-plugged.");
    setLowPower();
  }
  if (axp.isPEKShortPressIRQ()) {
    ESP_LOGI(TAG, "Short PEK press");
  }

  axp.clearIRQ();
}

void Power::setLowPower() {
  ESP_LOGD(TAG, "set low power");
  axp.setPowerOutPut(AXP192_LDO2, AXP202_OFF); //LORA
  axp.setPowerOutPut(AXP192_LDO3, AXP202_OFF); //GPS
  axp.setPowerOutPut(AXP192_DCDC2, AXP202_ON); //WARNING ESP
  axp.setPowerOutPut(AXP192_EXTEN, AXP202_OFF); //Extern
  axp.setPowerOutPut(AXP192_DCDC1, AXP202_OFF); //OLED
  //TODO: manage WIFI & BT here ?
}

void Power::setFullPower() {
  ESP_LOGD(TAG, "set Full power");
  axp.setPowerOutPut(AXP192_LDO2, AXP202_ON); //LORA
  axp.setPowerOutPut(AXP192_LDO3, AXP202_ON); //GPS
  axp.setPowerOutPut(AXP192_DCDC2, AXP202_ON); //WARNING ESP
  axp.setPowerOutPut(AXP192_EXTEN, AXP202_OFF); //Extern
  axp.setPowerOutPut(AXP192_DCDC1, AXP202_OFF); //OLED
  //TODO: manage WIFI & BT here ?
}

void Power::powerTest() {

  ESP_LOGI(TAG, "Batt voltage: %.3f%v", axp.getBattVoltage() / 1000);

}
