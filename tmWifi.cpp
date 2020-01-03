#include "tmWifi.h"


static const char TAG[] = __FILE__;

WiFiMulti WiFiMulti;

tmWifi::tmWifi() {
  ESP_LOGD(TAG, "tmWifi constructor");
}

void tmWifi::init() {
  ESP_LOGD(TAG, "Wifi init done.");
  WiFi.mode(WIFI_OFF); //TODO: check how to best optimize wifi power
}

void tmWifi::homeConnect() {
  ESP_LOGI(TAG, "request connect Wifi Home");
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(HOME_SSID, HOME_SSID_WPA);
  // wait for WiFi connection
  ESP_LOGD("Waiting for WiFi to connect...");
  while ((WiFiMulti.run() != WL_CONNECTED));
  setClock();
  ESP_LOGI(TAG, "connected Home");
}

void tmWifi::homeDisconnect() {
  WiFi.mode(WIFI_OFF);
  ESP_LOGI(TAG, "Dis-connected Home");
}

void tmWifi::setClock() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  ESP_LOGD(TAG, "Waiting for NTP time sync: ");
  time_t nowSecs = time(nullptr);
  while (nowSecs < 8 * 3600 * 2) {
    delay(500);
    yield();
    nowSecs = time(nullptr);
  }

  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);
  ESP_LOGD(TAG, "Current time: %s", asctime(&timeinfo)); 
}
