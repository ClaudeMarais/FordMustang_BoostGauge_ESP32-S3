#ifndef _SHARED
#define _SHARED

#ifdef DEBUG
#define DebugPrintf(...) Serial.printf(__VA_ARGS__)
#define DebugPrintln(...) Serial.println(__VA_ARGS__)
#else
#define DebugPrintln(...)
#define DebugPrintf(...)
#endif

// ESP32-S3 pins
#define ILI9341_SCK       SCK
#define ILI9341_MOSI      MOSI
#define ILI9341_MISO      GFX_NOT_DEFINED
#define ILI9341_CS        SS
#define ILI9341_DC        MISO
#define ILI9341_BL        D0
#define ILI9341_RST       D1
#define MOSFET_GATE       D2
#define FT6336_INT        D3
#define SN65HVD230_RXPin  D4
#define SN65HVD230_TXPin  D5
#define FT6336_RST        D6

// Car data needed for the information to be displayed on LCD
struct CarData
{
  int32_t EngineRPM;
  int32_t CurrentGear;
  int32_t GearboxMode;
  int32_t VehicleSpeed;
  int32_t TurboBoostPSI;
  int32_t DayNightStatus;
};

// This data is shared between two ESP32-S3 cores
CarData g_CurrentCarData { 0 };
SemaphoreHandle_t g_SemaphoreCarData = nullptr;
TaskHandle_t g_TaskDisplayInfo = nullptr;

#endif  // _SHARED
