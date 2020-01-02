/*
  //////////////////////// ESP32-Paxcounter \\\\\\\\\\\\\\\\\\\\\\\\\\
  // Tasks and timers:
  Task          Core  Prio  Purpose
  -------------------------------------------------------------------------------
  ledloop       0     3     blinks LEDs
  spiloop       0     2     reads/writes data on spi interface
  IDLE          0     0     ESP32 arduino scheduler -> runs wifi sniffer
  lmictask      1     2     MCCI LMiC LORAWAN stack
  clockloop     1     4     generates realtime telegrams for external clock
  timesync_req  1     3     processes realtime time sync requests
  taskISR       1     1     cyclic tasks (i.e. displayrefresh) triggered by timers
  gpsloop       1     1     reads data from GPS via serial or i2c
  lorasendtask  1     1     feeds data from lora sendqueue to lmcic
  IDLE          1     0     ESP32 arduino scheduler -> runs wifi channel rotator
  Low priority numbers denote low priority tasks.

*/

static const char TAG[] = __FILE__;

#include "globals.h"
#include "power.h"
#include "tmgps.h"
#include "tmWifi.h"
#include "tmLora.h"
#include "tmBT.h"

TaskHandle_t taskISRHandler = NULL;
TaskHandle_t taskGPSHandler = NULL;
TaskHandle_t TaskWifiHomeHandler = NULL;
TaskHandle_t TaskLoraHandler = NULL;
TaskHandle_t TaskBTHandler = NULL;

Power pwr; //Main Power instance for PMU
tmGPS gps; //main GPS init
tmWifi wifi; //Wifi management
tmLora lora;
tmBT BT;

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize serial communication at 115200 bits per second:
  /*Serial.begin(115200);
  Serial.println(F("TM tracker"));*/

  ESP_LOGD(TAG, "Main start");

  pwr.init();
  pwr.powerTest();

  gps.init();

  wifi.init();

  lora.init();
  lora.loraSend(); //TODO: à planifier

  BT.init();

  // Now set up two tasks to run independently.
  xTaskCreatePinnedToCore(
    taskISR
    ,  "taskISR"   // A name just for humans
    ,  2048  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  &taskISRHandler
    ,  CPU_CORE_1);

  xTaskCreatePinnedToCore(
    TaskGpsRead
    ,  "TaskGpsRead"
    ,  2048  // Stack size
    ,  NULL
    ,  1  // Priority
    ,  &taskGPSHandler
    ,  CPU_CORE_1);

  xTaskCreatePinnedToCore(
    TaskWifiHome
    ,  "TaskWifiHome"
    ,  3072  // Stack size
    ,  NULL
    ,  1  // Priority
    ,  &TaskWifiHomeHandler
    ,  CPU_CORE_1);

  xTaskCreatePinnedToCore(
    TaskLora
    ,  "TaskLora"
    ,  2048  // Stack size
    ,  NULL
    ,  1  // Priority
    ,  &TaskLoraHandler
    ,  CPU_CORE_0);

    xTaskCreatePinnedToCore(
    TaskBT
    ,  "TaskBT"
    ,  3072  // Stack size
    ,  NULL
    ,  1  // Priority
    ,  &TaskBTHandler
    ,  CPU_CORE_0);

  // Now the task scheduler, which takes over control of scheduling individual tasks, is automatically started.
}

void loop()
{
  // Empty. Things are done in Tasks.
}

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/

void taskISR(void *pvParameters)  // This is a task.
{
  static uint32_t InterruptStatus = 0x00;
  static uint32_t lastStack = ULONG_MAX;

  for (;;) // A Task shall never return or exit.
  {
    xTaskNotifyWait(0x00,             // Don't clear any bits on entry
                    ULONG_MAX,        // Clear all bits on exit
                    &InterruptStatus, // Receives the notification value
                    portMAX_DELAY);   // wait forever

    if (InterruptStatus & AXP_IRQ) {
      pwr.processIRQ();
      InterruptStatus &= ~AXP_IRQ;
    }

    int tmpStack = uxTaskGetStackHighWaterMark(NULL);
    if (tmpStack < lastStack) {
      lastStack = tmpStack;
      ESP_LOGD(TAG, "stack left: %d", tmpStack);
    }
  }
}

void TaskGpsRead(void *pvParameters)  // This is a task.
{
  (void) pvParameters;
  static uint32_t lastStack = ULONG_MAX;

  for (;;)
  {
    gps.readGPS();
    delay(4);
    if (gps.isFixed()) {

      //detect if home and connect
      gps.wifiHome();
    }

    int tmpStack = uxTaskGetStackHighWaterMark(NULL);
    if (tmpStack < lastStack) {
      lastStack = tmpStack;
      ESP_LOGD(TAG, "stack left: %d", tmpStack);
    }
  }
}


void TaskWifiHome(void *pvParameters)  // This is a task.
{
  (void) pvParameters;

  static uint32_t WifiStatus = 0x00;
  static uint32_t lastStack = ULONG_MAX;

  for (;;)
  {
    xTaskNotifyWait(0x00,             // Don't clear any bits on entry
                    ULONG_MAX,        // Clear all bits on exit
                    &WifiStatus, // Receives the notification value
                    portMAX_DELAY);   // wait forever

    if (WifiStatus & WIFI_HOME_ON) {
      wifi.homeConnect();
      WifiStatus &= ~WIFI_HOME_ON;
    }

    if (WifiStatus & WIFI_HOME_OFF) {
      wifi.homeDisconnect();
      WifiStatus &= ~WIFI_HOME_OFF;
    }

    int tmpStack = uxTaskGetStackHighWaterMark(NULL);
    if (tmpStack < lastStack) {
      lastStack = tmpStack;
      ESP_LOGD(TAG, "stack left: %d", tmpStack);
    }
  }
}

void TaskLora(void *pvParameters)  // This is a task.
{
  (void) pvParameters;
  static uint32_t lastStack = ULONG_MAX;

  for (;;)
  {
    os_runloop_once();
    delay(2);

    int tmpStack = uxTaskGetStackHighWaterMark(NULL);
    if (tmpStack < lastStack) {
      lastStack = tmpStack;
      ESP_LOGD(TAG, "stack left: %d", tmpStack);
    }
  }
}

void TaskBT(void *pvParameters)  // This is a task.
{
  (void) pvParameters;
  static uint32_t lastStack = ULONG_MAX;

  BT.connect();
  
  for (;;)
  {
    BT.keepAlive();
    vTaskDelay( BT_KEEP_ALIVE_INTERVAL / portTICK_PERIOD_MS );

    int tmpStack = uxTaskGetStackHighWaterMark(NULL);
    if (tmpStack < lastStack) {
      lastStack = tmpStack;
      ESP_LOGD(TAG, "stack left: %d", tmpStack);
    }
  }
}
