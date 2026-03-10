// Display info on a ILI9341V SPI display, 2.8 inch 240x320 Capacitive Touch Screen

#ifndef _DISPLAY_INFO
#define _DISPLAY_INFO

#include <Arduino_GFX_Library.h>
#include "FreeMonoBold54pt7b.h"
#include "FreeMonoBold18pt7b.h"

// Setup gfx library for the ILI9341 display
Arduino_DataBus* bus = new Arduino_ESP32SPI(ILI9341_DC, ILI9341_CS, ILI9341_SCK, ILI9341_MOSI, ILI9341_MISO);
Arduino_GFX* gfx = new Arduino_ILI9341(bus, ILI9341_RST, 0, true);
bool bIsDisplayOn = false;

// Define a simple shift indicator based only on RPM. It would be better to setup something using dyno numbers, looking at torque, etc.
struct ShiftIndicatorLights
{
  int32_t EngineRPM;
  uint16_t Color;
};

const int32_t NumLights = 5;
const int32_t ShiftNowIndex = 0;  // Index into shiftIndicatorLights for ideal shift time, i.e. when the indicator turns red
const ShiftIndicatorLights shiftIndicatorLights[NumLights] = { {5800, RGB565_RED}, {4500, RGB565_YELLOW}, {3000, RGB565_GREEN}, {1500, RGB565_BLUE}, {0, RGB565_DARKGREY} };

const int32_t MaxRPM  = 6800;               // Max RPM to show on LCD
const int32_t Reverse = -1;                 // Define Reverse as a special gear number
const int32_t Park    = -2;                 // Define Park as a special gear number
int32_t  previousGear = -12;                // A randomly chosen initial number
uint16_t previousGearColor = RGB565_WHITE;
int32_t  previousSpeed = -1;                // Previous speed value for efficient updating
uint16_t previousSpeedColor = RGB565_WHITE;
int32_t  previousTurboBoost = -999;         // Previous turbo boost value for efficient updating
uint16_t previousTurboBoostColor = RGB565_WHITE;

// Peak-hold for turbo boost: holds the max reading for a short time after boost drops (e.g. gear shift)
const unsigned long BoostPeakHoldMs = 500;   // How long to hold the peak value (ms)
int32_t  boostPeakValue = 0;                 // Current peak boost being held
unsigned long boostPeakTime = 0;             // millis() when the peak was first recorded
bool prevAboveShiftThreshold = false;        // Tracks whether RPM was above shift-now threshold last frame

// Night mode state - triggered by CAN 0x3B3 Day_Night_Status signal
bool g_NightMode = false;

// Dim a RGB565 color for night mode (reduces brightness to ~35%)
uint16_t DimColor(uint16_t color)
{
  if (!g_NightMode)
  {
    return color;
  }

  uint8_t r5 = (color >> 11) & 0x1F;
  uint8_t g6 = (color >> 5) & 0x3F;
  uint8_t b5 = color & 0x1F;
  r5 = (r5 * 35) / 100;
  g6 = (g6 * 35) / 100;
  b5 = (b5 * 35) / 100;

  return (r5 << 11) | (g6 << 5) | b5;
}

// Interrupt handler for when touch was detected on the screen
static volatile uint8_t touched = false;
static volatile unsigned long touchedTime = 0;
void ft6336IntHandler(void)
{
  touched = true;
  touchedTime = millis();
}

// g_CurrentCarData is populated by the SN65HVD230 transceiver on another ESP32-S3 core. Since the data needs to be thread safe, we keep a
// local copy of the data on this thread, copying it safely using g_SemaphoreCarData
CarData carData;

// Make a local copy of car data that was gathered on the other ESP32-S3 core
void CopyCarData()
{
  if (xSemaphoreTake(g_SemaphoreCarData, pdMS_TO_TICKS(100)) != pdTRUE)
  {
    return;
  }

  memcpy(&carData, &g_CurrentCarData, sizeof(CarData));
  xSemaphoreGive(g_SemaphoreCarData);

  // If Neutral, then check if we're in Park, since we'd rather display "P" than "N" when in Park
  if (carData.CurrentGear == 0 &&
      carData.GearboxMode == GearboxMode::P)
  {
    carData.CurrentGear = Park;
  }
}

// Turn power for display ON
void TurnMosfetSwitchOn()
{
  DebugPrintln("Turn Mosfet switch ON");
  pinMode(MOSFET_GATE, OUTPUT);
  digitalWrite(MOSFET_GATE, LOW);   // Gate = 0V
}

// Turn power for display OFF
// IMPORTANT: A 3.3V HIGH from the ESP32 pin is NOT enough to turn off a PMOS whose source is at 5V
//            (3.3V gate gives Vgs = -1.7V → still partially ON)
// Correct control method with a 5V source:
//   - Use a 2.2K pull-up resistor from Gate → Source (5V)
//   - Drive the ESP32 pin in "open-drain style":
//            pinMode(PIN, INPUT);   // Hi-Z, pull-up forces Gate = 5V
void TurnMosfetSwitchOff()
{
  DebugPrintln("Turn Mosfet switch OFF");
  //pinMode(MOSFET_GATE, OUTPUT);
  //digitalWrite(MOSFET_GATE, HIGH);
  pinMode(MOSFET_GATE, INPUT);
}

// Draw static labels that never change (called once during initialization)
void DrawStaticLabels()
{
  const int16_t width = 320;
  const int16_t height = 240;
  const int16_t largeFontSize = 54;
  const int32_t bottom = height - (largeFontSize / 4);

  gfx->setFont(&FreeMonoBold18pt7b);
  gfx->setTextSize(1);

  int16_t mphLabelX = width - (int16_t)(largeFontSize * 2.4f) + largeFontSize - 20;
  int16_t mphLabelY = bottom - (largeFontSize / 2) - (18 * 3);
  int16_t psiLabelX = 0 + largeFontSize - 18;
  int16_t psiLabelY = bottom - (largeFontSize / 2) - (18 * 3);

  // Erase old labels (harmless on first call since screen is already black)
  gfx->setTextColor(RGB565_BLACK);
  gfx->setCursor(mphLabelX, mphLabelY);
  gfx->printf("mph");
  gfx->setCursor(psiLabelX, psiLabelY);
  gfx->printf("psi");

  // Draw labels with current color (dimmed in night mode)
  gfx->setTextColor(DimColor(RGB565_NAVY));
  gfx->setCursor(mphLabelX, mphLabelY);
  gfx->printf("mph");
  gfx->setCursor(psiLabelX, psiLabelY);
  gfx->printf("psi");
}

void TurnBacklightOn()
{
  pinMode(ILI9341_BL, OUTPUT);
  digitalWrite(ILI9341_BL, HIGH);
}

void TurnBacklightOff()
{
  pinMode(ILI9341_BL, OUTPUT);
  digitalWrite(ILI9341_BL, LOW);
}

void TurnDisplayOn()
{
  DebugPrintln("TurnDisplayOn()");

  // Turn power on
  TurnMosfetSwitchOn();
  delay(20);

  // Init display
  while (!gfx->begin())
  {
    DebugPrintln("gfx->begin() failed!");
    delay(500);
  }

  TurnBacklightOn();

  bIsDisplayOn = true;

  gfx->fillScreen(RGB565_BLACK);
  gfx->setRotation(3);
  gfx->setTextSize(1);

  pinMode(FT6336_INT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FT6336_INT), ft6336IntHandler, FALLING);

  DrawStaticLabels();
}

void TurnDisplayOff()
{
  DebugPrintln("TurnDisplayOff()");
  detachInterrupt(digitalPinToInterrupt(FT6336_INT));
  gfx->fillScreen(RGB565_BLACK);
  gfx->displayOff();

  TurnBacklightOff();

  // Set all display pins to INPUT to minimize power consumption and avoid floating outputs
  pinMode(ILI9341_SCK, INPUT);
  pinMode(ILI9341_MOSI, INPUT);
  pinMode(ILI9341_CS, INPUT);
  pinMode(ILI9341_DC, INPUT);
  pinMode(ILI9341_RST, INPUT);
  pinMode(ILI9341_BL, INPUT);
  pinMode(FT6336_INT, INPUT);
  pinMode(FT6336_RST, INPUT);

  delay(20);

  TurnMosfetSwitchOff();

  bIsDisplayOn = false;
  delay(100);
}

char GenerateGearText(const int32_t gear)
{
  if ((gear > 0) && (gear <= 8))
  {
    return '0' + gear;
  }
  else
  {
    return (gear == Park) ? 'P' : (gear == Reverse) ? 'R' : (gear == 0) ? 'N' : 'D';
  }
}

void DrawGearText(const int16_t x, const int16_t y, const GFXfont* pFont, const int16_t fontSize)
{
  // Determine and keep track of the gear color (dimmed in night mode)
  uint16_t color = DimColor(RGB565_WHITE);
  if (carData.EngineRPM > shiftIndicatorLights[ShiftNowIndex].EngineRPM)
  {
    color = DimColor(RGB565_RED);
  }

  gfx->setFont(pFont);

  // Update the text when the gear changes
  if (carData.CurrentGear != previousGear)
  {
    // Clear the previous gear text by redrawing it in black
    gfx->setCursor(x - (fontSize / 2), y);
    gfx->setTextColor(RGB565_BLACK);
    gfx->printf("%c", GenerateGearText(previousGear));

    // Show the new gear
    gfx->setCursor(x - (fontSize / 2), y);
    gfx->setTextColor(color);
    gfx->printf("%c", GenerateGearText(carData.CurrentGear));

    previousGear = carData.CurrentGear;
    previousGearColor = color;
  }

  // Update the text when the gear color changes
  if (color != previousGearColor)
  {
    // Show the current gear with the new color
    gfx->setCursor(x - (fontSize / 2), y);
    gfx->setTextColor(color);
    gfx->printf("%c", GenerateGearText(carData.CurrentGear));

    previousGearColor = color;
  }
}

void DrawSpeedText(int16_t x, int16_t y, int16_t fontSize)
{
  // Convert km/h to mph
  int32_t speedMPH = int32_t(carData.VehicleSpeed / 1.60934f + 0.5f);
  uint16_t color = DimColor(RGB565_WHITE);

  gfx->setFont(&FreeMonoBold54pt7b);
  gfx->setTextSize(1);

  if (speedMPH != previousSpeed)
  {
    // Clear the previous speed text by redrawing it in black
    if (previousSpeed >= 0)
    {
      gfx->setCursor(x, y);
      gfx->setTextColor(RGB565_BLACK);
      gfx->printf("%d", previousSpeed);
    }

    // Show the new speed
    gfx->setCursor(x, y);
    gfx->setTextColor(color);
    gfx->printf("%d", speedMPH);

    previousSpeed = speedMPH;
    previousSpeedColor = color;
  }

  // Update when color changes (e.g., night mode toggle)
  if (color != previousSpeedColor)
  {
    gfx->setCursor(x, y);
    gfx->setTextColor(color);
    gfx->printf("%d", speedMPH);
    previousSpeedColor = color;
  }
}

void DrawTurboBoostText(int16_t x, int16_t y, int16_t fontSize)
{
  // Get current turbo boost PSI value
  int32_t turboBoostPSI_int = carData.TurboBoostPSI;

  // Clamp value, only show boost, vacuum is not relevant for display
  if (turboBoostPSI_int < 0)
  {
    turboBoostPSI_int = 0;
  }

  // Peak-hold logic: when boost drops after a peak, keep showing the peak for BoostPeakHoldMs
  // Rising values always update immediately so the display feels responsive during acceleration
  unsigned long now = millis();

  if (turboBoostPSI_int >= boostPeakValue)
  {
    // Boost is rising or steady — update peak and timestamp
    boostPeakValue = turboBoostPSI_int;
    boostPeakTime = now;
  }
  else
  {
    // Boost dropped — hold the peak for a short time
    if ((now - boostPeakTime) < BoostPeakHoldMs)
    {
      turboBoostPSI_int = boostPeakValue;  // keep showing peak
    }
    else
    {
      // Hold expired — accept the new (lower) value as the new peak
      boostPeakValue = turboBoostPSI_int;
      boostPeakTime = now;
    }
  }
  
  // Determine turbo boost color based on boost level (dimmed in night mode)
  uint16_t color = DimColor(RGB565_WHITE);
  if (turboBoostPSI_int > 10)
  {
    color = DimColor(RGB565_RED);  // High boost
  }
  else if (turboBoostPSI_int > 5)
  {
    color = DimColor(RGB565_YELLOW);  // Medium boost
  }

  gfx->setFont(&FreeMonoBold54pt7b);
  gfx->setTextSize(1);

  // Update the text when the turbo boost changes
  if (turboBoostPSI_int != previousTurboBoost)
  {
    // Clear the previous turbo boost text by redrawing it in black
    if (previousTurboBoost != -999)  // Only erase if we have a valid previous value
    {
      gfx->setCursor(x, y);
      gfx->setTextColor(RGB565_BLACK);
      gfx->printf("%d", previousTurboBoost);
    }

    // Show the new turbo boost
    gfx->setCursor(x, y);
    gfx->setTextColor(color);
    gfx->printf("%d", turboBoostPSI_int);

    previousTurboBoost = turboBoostPSI_int;
    previousTurboBoostColor = color;
  }

  // Update the text when the turbo boost color changes
  if (color != previousTurboBoostColor)
  {
    // Show the current turbo boost with the new color
    gfx->setFont(&FreeMonoBold54pt7b);
    gfx->setTextSize(1);
    gfx->setCursor(x, y);
    gfx->setTextColor(color);
    gfx->printf("%d", turboBoostPSI_int);

    previousTurboBoostColor = color;
  }
}

void DrawShiftIndicator(int16_t x, int16_t y, int16_t radius)
{
  uint16_t color = DimColor(RGB565_DARKGREY);

  for (int i = 0; i < NumLights; i++)
  {
    if (carData.EngineRPM > shiftIndicatorLights[i].EngineRPM)
    {
      color = DimColor(shiftIndicatorLights[i].Color);
      break;
    }
  }

  const uint16_t rotate = 90;
  const uint16_t endRadius = (min(MaxRPM, carData.EngineRPM) / float(MaxRPM)) * 180.0f;

  // Draw left side bar
  gfx->fillArc(x, y, radius + 10, radius - 10, 0 + rotate, endRadius + rotate, color);
  gfx->fillArc(x, y, radius + 10, radius - 10, endRadius + rotate, 180 + rotate, RGB565_BLACK);

  // Draw right side bar
  gfx->fillArc(x, y, radius + 10, radius - 10, 360 - endRadius + rotate, 360 + rotate, color);
  gfx->fillArc(x, y, radius + 10, radius - 10, 360 - 180 + rotate, 360 - endRadius + rotate, RGB565_BLACK);
}

// Main function of the thread task running on a separate ESP32-S3 core
void DisplayInfo(void* params)
{
  DebugPrintf("Core %d: DisplayInfo()\n", xPortGetCoreID());

  TurnDisplayOn();

  while (true)
  {
    CopyCarData();

    if (bIsDisplayOn)
    {
      if (touched && (millis() - touchedTime) > 50)
      {
        touched = false;

        // Toggle backlight on/off
        static bool backlightOn = true;
        backlightOn = !backlightOn;
        backlightOn ? TurnBacklightOn() : TurnBacklightOff();
      }

      // Check for night mode change (CAN 0x3B3: Day_Night_Status 2=Night)
      bool nightMode = (carData.DayNightStatus == 2);
      if (nightMode != g_NightMode)
      {
        g_NightMode = nightMode;
        DrawStaticLabels();  // Redraw labels with new color
      }
     
      // Continue normal display operations
      const int16_t width = 320;
      const int16_t height = 240;
      const int16_t centerWidth = width / 2;
      const int16_t largeFontSize = 54;
      const int32_t bottom = height - (largeFontSize / 4);

      // Flash entire screen red when RPM crosses the shift-now threshold (upward only)
      bool aboveShiftThreshold = (carData.EngineRPM > shiftIndicatorLights[ShiftNowIndex].EngineRPM);
      if (aboveShiftThreshold && !prevAboveShiftThreshold)
      {
        gfx->fillScreen(RGB565_RED);
        delay(250);
        gfx->fillScreen(RGB565_BLACK);

        // Force full redraw of everything after the flash
        DrawStaticLabels();
        previousGear = -12;
        previousGearColor = RGB565_WHITE;
        previousSpeed = -1;
        previousSpeedColor = RGB565_WHITE;
        previousTurboBoost = -999;
        previousTurboBoostColor = RGB565_WHITE;
      }
      prevAboveShiftThreshold = aboveShiftThreshold;

      DrawGearText(centerWidth, 90, &FreeMonoBold54pt7b, largeFontSize);
      DrawShiftIndicator(centerWidth, 60, 60);
      DrawSpeedText(width - (int16_t)(largeFontSize * 2.4f), bottom, largeFontSize);
      DrawTurboBoostText(0, bottom, largeFontSize);
    }

    delay(1);  // Make sure something is happening on the thread, otherwise a watchdog timer will kick in after 5 seconds and reboot device
  }
}

#endif  // _DISPLAY_INFO