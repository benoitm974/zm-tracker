#include "globals.h"
#include "arduino.h"
#include "tmgps.h"

static const char TAG[] = __FILE__;

HardwareSerial GPS(1); // use UART #1

tmGPS::tmGPS() {
  ESP_LOGD(TAG, "GPS constructor");
}

void tmGPS::init() {
  GPS.begin(9600, SERIAL_8N1, 34, 12);
  ESP_LOGD(TAG, "GPS init done.");
}

void tmGPS::gpsTest() {

  ESP_LOGI(TAG, "gps test");

}

void tmGPS::wifiHome() {
  ESP_LOGI(TAG, "wifi Home");
  //TODO: if within range
  //WIFI_HOME_ON
  //else
  //WIFI_HOME_OFF
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  xTaskNotifyFromISR(TaskWifiHomeHandler, WIFI_HOME_ON, eSetBits,
                     &xHigherPriorityTaskWoken);

  if (xHigherPriorityTaskWoken)
    portYIELD_FROM_ISR();
}

void tmGPS::readGPS() {
  while (GPS.available()) {
    gps.encode(GPS.read());
  }
}

bool tmGPS::isFixed() {
  static bool lastStatus = false;

  if (gps.location.isValid() &&
      gps.location.age() < 2000 &&
      gps.hdop.isValid() &&
      gps.hdop.value() <= 300 &&
      gps.hdop.age() < 2000 &&
      gps.altitude.isValid() &&
      gps.altitude.age() < 2000 )
  {
    if (!lastStatus)
      ESP_LOGI(TAG, "Valid gps Fix.");
    lastStatus = true;
  } else {
    if (lastStatus)
      ESP_LOGI(TAG, "gps not fix");
    lastStatus = false;
  }
  return lastStatus;
}
