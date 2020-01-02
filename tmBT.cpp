#include "globals.h"
#include "tmBT.h"

static const char TAG[] = __FILE__;

uint8_t address[6]  = BTAddress;
String name = BTName;

tmBT::tmBT() {
  ESP_LOGD(TAG, "BT constructor");
}

void tmBT::init() {
  ESP_LOGD(TAG, "BT init done.");
}

void tmBT::connect() {

  if (connected) {
    ESP_LOGD(TAG, "BT already connected.");
  } else {
    SerialBT.begin("zm-tracker", true);

    // connect(address) is fast (upto 10 secs max), connect(name) is slow (upto 30 secs max) as it needs
    // to resolve name to address first, but it allows to connect to different devices with the same name.
    // Set CoreDebugLevel to Info to view devices bluetooth address and device names
    //connected = SerialBT.connect(name);
    connected = SerialBT.connect(address);

#ifdef LOG_TO_BT
    esp_log_set_vprintf(debugOutputHandler); //TODO: how to link to static fucntion from class. & activate the define in the globals.h
#endif

    ESP_LOGD(TAG, "BT connected %d", connected);
  }

}

void tmBT::keepAlive() {
  int nHex[] = { 0xf1, 0xf2, 0xf4, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x47, 0x62, 0x6b, 0x69, 0xf8, 0xf4, 0xf2, 0xf1, 0x81, 0x9e, 0x77, 0xc5 };
  //int nHex = 0xf1f2f4f80000000047626b69f8f4f2f1819e77c5;
  if (!connected) {
    ESP_LOGI(TAG, "BT keepAlive had to reconnect.");
    SerialBT.connect();
  }
  for (int i = 0; i < sizeof(nHex); i++) {
    SerialBT.print(nHex[i]);

    //SerialBT.println("keep alive");
    //ESP_LOGI(TAG,"%x",nHex[i]);
  }

}
