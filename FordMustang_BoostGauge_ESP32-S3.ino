// An Arduino project boost gauge for a 2016 Ford Mustang Ecoboost. OBD2 data is used to display turbo boost pressure, current gear, vehicle speed and a shift indicator graph.
//
// Hardware:
//    XIAO ESP32-S3
//    12V to 5V Voltage regulator
//    SN65HVD230 CAN bus transceiver
//    ILI9341V SPI display, 2.8 inch 240x320 Capacitive Touch Screen
//    AO3401 A19T P-Channel Mosfet (30V 4.2A RDS 50mΩ)
//    220 Ohm Resistor
//    2.2K Ohm Resistor
//    Magnetic Pogo Pin Connector
//    OBD2 Connector
//    USB Data Cable
//
// Arduino library used:
//    ESP32-TWAI-CAN: https://github.com/handmade0octopus/ESP32-TWAI-CAN
//    ArduinoGFX: https://github.com/moononournation/Arduino_GFX

// The final build should have this commented out
//#define DEBUG 1           // When this is defined, Serial input/output will happen, otherwise not
//#define DEBUG_RPM 1       // Allows for typing in numbers [0..7] in Serial Monitor to emulate RPM
//#define DEBUG_GEAR 1      // Allows for typing in numbers [0..7] in Serial Monitor to emulate current gear. This also requires DISABLE_POWER_SAVING
//#define DEBUG_MPH 1       // Allows for typing in numbers [0..199] in Serial Monitor to emulate vehicle speed in MPH
//#define DEBUG_PSI 1       // Allows for typing in numbers [0..30] in Serial Monitor to emulate turbo boost PSI

// The final build should have this commented out
// It's sometimes easier to debug without the device going into power save mode
//#define DISABLE_POWER_SAVING 1

#include "Shared.h"
#include "CollectCarData.h"
#include "DisplayInfo.h"
#include "HandleLowPowerState.h"


void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW); // Turn on-board LED on

#ifdef DEBUG
  Serial.begin(115200);
  delay(200);
  Serial.printf("\n\n********* OBD2 Display for Ford Mustang using ESP32-S3 *********\n\n");
#endif

  // Device boots with 240Mhz clock speed. To save power we'll switch to 40Mhz, but just in case the device goes into a
  // reboot loop because of the lower clock, we delay for a few seconds, which will give time to reflash the program
  DebugPrintf("Clock frequency = %d\n", getCpuFrequencyMhz());
  DebugPrintf("Wait 4 seconds just in case device crashes with %dMHz\n", ClockSpeed_LiteSleep);
  delay(4000);

  // Setup the CAN transceiver to receive CAN frames 
  SetupCollectCarData();

  // Wait until the car turns ON
  WaitForCarToTurnOn();

  // Create task that will display info using ESP32-S3 core 0
  xTaskCreatePinnedToCore(DisplayInfo, nullptr, 1024 * 128, nullptr, 1, &g_TaskDisplayInfo, 0);
}

void loop()
{
  CollectCarData();
  CheckIfCarIsStillOn();
}
