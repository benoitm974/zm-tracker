#include "tmLora.h"

static const char TAG[] = __FILE__;

// Those variables keep their values after software restart or wakeup from sleep, not after power loss or hard reset !
RTC_NOINIT_ATTR int RTCseqnoUp, RTCseqnoDn;
#ifdef USE_OTAA
RTC_NOINIT_ATTR u4_t otaaDevAddr;
RTC_NOINIT_ATTR u1_t otaaNetwKey[16];
RTC_NOINIT_ATTR u1_t otaaApRtKey[16];
#endif

uint8_t txBuffer[9] = "hello123";
static osjob_t sendjob;


// Schedule TX every this many seconds (might become longer due to duty cycle limitations).
const unsigned TX_INTERVAL = 120; //TODO used ?

// Pin mapping for TBeams, might not suit the latest version > 1.0 ?
const lmic_pinmap lmic_pins = {
  .nss = 18,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = LMIC_UNUSED_PIN, // was "14,"
  .dio = {26, 33, 32},
};

// These callbacks are only used in over-the-air activation.
#ifdef USE_OTAA
void os_getDevEui (u1_t* buf) {
  memcpy_P(buf, DEVEUI, 8);
}
void os_getArtEui (u1_t* buf) {
  memcpy_P(buf, APPEUI, 8);
}
void os_getDevKey (u1_t* buf) {
  memcpy_P(buf, APPKEY, 16);
}
#else
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }
#endif

tmLora::tmLora() {
  ESP_LOGD(TAG, "Lora constructor");
}

void storeFrameCounters()
{
  RTCseqnoUp = LMIC.seqnoUp;
  RTCseqnoDn = LMIC.seqnoDn;
  ESP_LOGD(TAG, "Counters stored as %d/%d", LMIC.seqnoUp, LMIC.seqnoDn);
}

void restoreFrameCounters()
{
  LMIC.seqnoUp = RTCseqnoUp;
  LMIC.seqnoDn = RTCseqnoDn;
  ESP_LOGD(TAG, "Restored counters as %d/%d", LMIC.seqnoUp, LMIC.seqnoDn);
}

void setOrRestorePersistentCounters()
{
  esp_reset_reason_t reason = esp_reset_reason();
  if ((reason != ESP_RST_DEEPSLEEP) && (reason != ESP_RST_SW))
  {
    ESP_LOGI(TAG, "Counters both set to 0");
    LMIC.seqnoUp = 0;
    LMIC.seqnoDn = 0;
  }
  else
  {
    restoreFrameCounters();
  }
}

void onEvent (ev_t ev) {
  switch (ev) {
    case EV_SCAN_TIMEOUT:
      ESP_LOGI(TAG, "EV_SCAN_TIMEOUT");
      break;
    case EV_BEACON_FOUND:
      ESP_LOGI(TAG, "EV_BEACON_FOUND");
      break;
    case EV_BEACON_MISSED:
      ESP_LOGI(TAG, "EV_BEACON_MISSED");
      break;
    case EV_BEACON_TRACKED:
      ESP_LOGI(TAG, "EV_BEACON_TRACKED");
      break;
    case EV_JOINING:
      ESP_LOGI(TAG, "EV_JOINING");
      break;
    case EV_JOINED:
      ESP_LOGI(TAG, "EV_JOINED");
#ifdef USE_OTAA
      otaaDevAddr = LMIC.devaddr;
      memcpy_P(otaaNetwKey, LMIC.nwkKey, 16);
      memcpy_P(otaaApRtKey, LMIC.artKey, 16);
      ESP_LOGD(TAG, "got devaddr = 0x%X", LMIC.devaddr);
#endif
      // Disable link check validation (automatically enabled
      // during join, but not supported by TTN at this time).
      LMIC_setLinkCheckMode(0);
      // TTN uses SF9 for its RX2 window.
      LMIC.dn2Dr = DR_SF9;
      break;
    case EV_RFU1:
      ESP_LOGI(TAG, "EV_RFU1");
      break;
    case EV_JOIN_FAILED:
      ESP_LOGE(TAG, "EV_JOIN_FAILED");
      break;
    case EV_REJOIN_FAILED:
      ESP_LOGE(TAG, "EV_REJOIN_FAILED");
      break;
    case EV_TXCOMPLETE:
      ESP_LOGI(TAG, "EV_TXCOMPLETE (includes waiting for RX windows)");
      if (LMIC.txrxFlags & TXRX_ACK) {
        ESP_LOGI(TAG, "Received Ack");
      }
      if (LMIC.dataLen) {
        ESP_LOGD(TAG, "Received %i bytes of payload", LMIC.dataLen);
        ESP_LOGI(TAG, "RSSI %d SNR %.1d", LMIC.rssi, LMIC.snr);
      }
      storeFrameCounters();
      // Schedule next transmission
      /* TODO:next transmission ?
        Serial.println("Good night...");
        axp.setPowerOutPut(AXP192_LDO2, AXP202_OFF);
        axp.setPowerOutPut(AXP192_LDO3, AXP202_OFF);
        esp_sleep_enable_timer_wakeup(TX_INTERVAL*1000000);
        esp_deep_sleep_start();
        do_send(&sendjob);*/
      break;
    case EV_LOST_TSYNC:
      ESP_LOGI(TAG, "EV_LOST_TSYNC");
      break;
    case EV_RESET:
      ESP_LOGI(TAG, "EV_RESET");
      break;
    case EV_RXCOMPLETE:
      // data received in ping slot
      ESP_LOGI(TAG, "EV_RXCOMPLETE");
      break;
    case EV_LINK_DEAD:
      ESP_LOGI(TAG, "EV_LINK_DEAD");
      break;
    case EV_LINK_ALIVE:
      ESP_LOGI(TAG, "EV_LINK_ALIVE");
      break;
    default:
      ESP_LOGI(TAG, "Unknown event");
      break;
  }
}

void tmLora::do_send(osjob_t* j) {

  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND)
  {
    ESP_LOGI(TAG, "OP_TXRXPEND, not sending");
  }
  else
  {
    /*if (gps.checkGpsFix())
    {*/
      // Prepare upstream data transmission at the next possible time.
      //TODO: gps.buildPacket(txBuffer);
      LMIC_setTxData2(1, txBuffer, sizeof(txBuffer), 0);
      ESP_LOGI(TAG, "Packet queued");
    /*}
    else
    {
      //try again in 3 seconds
      os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(3), do_send);
    }*/
  }
  // Next TX is scheduled after TX_COMPLETE event.
}

void tmLora::init() {
  ESP_LOGD(TAG, "Lora init start.");
  // LMIC init
  os_init();ESP_LOGD(TAG, "Lora init 1");
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();ESP_LOGD(TAG, "Lora init 2");


#ifdef USE_OTAA
  esp_reset_reason_t reason = esp_reset_reason();ESP_LOGD(TAG, "Lora init 3");
  if ((reason == ESP_RST_DEEPSLEEP) || (reason == ESP_RST_SW))
  {
    LMIC_setSession(0x1, otaaDevAddr, otaaNetwKey, otaaApRtKey);
  }
#else // ABP
  LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);

  // TTN uses SF9 for its RX2 window.
  LMIC.dn2Dr = DR_SF9;

  // Disable link check validation
  LMIC_setLinkCheckMode(0);
#endif

  // This must be done AFTER calling LMIC_setSession !
  setOrRestorePersistentCounters();ESP_LOGD(TAG, "Lora init 4");

#ifdef CFG_eu868
  LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
  LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band
#endif

#ifdef CFG_us915
  LMIC_selectSubBand(1);

  //Disable FSB2-8, channels 16-72
  for (int i = 16; i < 73; i++) {
    if (i != 10)
      LMIC_disableChannel(i);
  }
#endif
ESP_LOGD(TAG, "Lora init 5");
  // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
  LMIC_setDrTxpow(DR_SF7, 14);
  ESP_LOGD(TAG, "Lora init done.");
}

void tmLora::loraSend() {
  do_send(&sendjob);
}
