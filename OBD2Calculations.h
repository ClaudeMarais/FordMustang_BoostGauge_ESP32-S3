// Calculate car data values from OBD2 data. These calculations specifically work for the 2016 Ford Mustang Ecoboost. They might not work on other cars, feel free to experiment

#ifndef _OBD2_CALCULATIONS
#define _OBD2_CALCULATIONS

// --------------------------------------------------------
// ******** Engine RPM ************************************
// --------------------------------------------------------

static int32_t g_EngineRPM = 0;

int32_t CalcEngineRPM(const uint8_t* pData)
{
  uint8_t A = pData[3];
  uint8_t B = pData[4];
  g_EngineRPM = (A * 256 + B) * 2;

  return g_EngineRPM;
}

void PrintEngineRPM()
{
  Serial.printf("Engine RPM = %d\n", g_EngineRPM);
}

// --------------------------------------------------------
// ******** Vehicle Speed (km/h) **************************
// --------------------------------------------------------

static float g_VehicleSpeed = 0;

// VehOverGnd_V_Est: 7|16@0+ (0.01,0) kph on CAN 0x077 (BO_ 119)
int32_t CalcVehicleSpeed(const uint8_t* pData)
{
  uint16_t raw = (pData[0] << 8) | pData[1];
  g_VehicleSpeed = raw * 0.01f;
  return int32_t(g_VehicleSpeed + 0.5f);
}

void PrintVehicleSpeed()
{
  Serial.printf("Vehicle Speed = %d km/h (%d mph)\n", int32_t(g_VehicleSpeed + 0.5f), int32_t(g_VehicleSpeed / 1.60934f + 0.5f));
}

// --------------------------------------------------------
// ******** Turbo Boost Pressure (psi) ********************
// --------------------------------------------------------

static float g_TurboBoostPSI = 0;

// EngMnfld_P_Actl: 47|13@0+ (0.1,-206.8) kPa on CAN 0x43E (BO_ 1086 PowertrainData_3)
// Bytes 5-6, big-endian, 13-bit >>3, scale 0.1, offset -206.8, absolute pressure (no atmospheric subtraction)
int32_t CalcTurboBoostPressure(const uint8_t* pData)
{
  uint16_t raw = ((uint16_t)pData[5] << 8) | pData[6];
  raw >>= 3;
  raw &= 0x1FFF;

  float kPa = raw * 0.1f - 206.8f;
  float boost_psi = kPa * 0.145038f;

  g_TurboBoostPSI = boost_psi;
  return (int32_t)roundf(boost_psi);
}

void PrintTurboBoostPressure()
{
  Serial.printf("Turbo Boost = %.1f PSI\n", g_TurboBoostPSI);
}

// --------------------------------------------------------
// ******** Currently Engaged Gear ************************
// --------------------------------------------------------

static int32_t g_CurrentGear = 0;    // 0 = Neutral, -1 = Reverse

int32_t CalcCurrentGear(const uint8_t* pData)
{
  if (pData[1] == 2)
  {
    g_CurrentGear = -1; // Reverse
  }
  else if (pData[1] == 4)
  {
    g_CurrentGear = 0;  // Neutral
  }
  else
  {
    g_CurrentGear = pData[0] / 16;
  }

  return g_CurrentGear;
}

void PrintCurrentGear()
{
  char gearStr[32] = "Neutral";

  if (g_CurrentGear == -1)
  {
    sprintf(gearStr, "Reverse");
  }
  else if (g_CurrentGear > 0)
  {
    sprintf(gearStr, "%d", g_CurrentGear);
  }

  Serial.printf("Current Engaged Gear = %s\n", gearStr);
}

// --------------------------------------------------------
// ******** Gearbox Mode **********************************
// --------------------------------------------------------

// Gearbox  mode
enum GearboxMode
{
  P = 0x00,   // Park
  R = 0x20,   // Reverse
  N = 0x40,   // Neutral
  D = 0x60,   // Drive
  S = 0x80,   // Sport
};

static int32_t g_GearboxMode = GearboxMode::P;

int32_t CalcGearboxMode(const uint8_t* pData)
{
  g_GearboxMode = pData[1];
  return g_GearboxMode;
}

void PrintGearboxMode()
{
  Serial.printf("Gearbox Mode: %#04x %s\n", g_GearboxMode, (g_GearboxMode == GearboxMode::P) ? "P" :
                                                           (g_GearboxMode == GearboxMode::R) ? "R" :
                                                           (g_GearboxMode == GearboxMode::N) ? "N" :
                                                           (g_GearboxMode == GearboxMode::D) ? "D" :
                                                           (g_GearboxMode == GearboxMode::S) ? "S" : "ERROR: Unknown Drive Mode");
}

// --------------------------------------------------------
// ******** Day/Night Status ******************************
// --------------------------------------------------------

static int32_t g_DayNightStatus = 0;   // 0=Unknown/Null, 1=Day, 2=Night

// Day_Night_Status from CAN 0x3B3 (BO_ 947), signal: 15|2@0+ (1,0)
// Byte 1, bits 7-6 (top 2 bits of byte 1)
//
// If 0x3B3 is not on the bus, alternative CAN IDs to try:
//   - CAN 0x083 (131) LghtAmb_D_Sns: ambient light sensor, byte 1 bits 7-5 (3-bit), 0=Dark 1=Light 2=Twilight
//   - CAN 0x3C3 (963) HeadLghtSwtch_D_Stat: headlight switch, byte 1 bits 7-6 (2-bit), 0=Off 1=Parklamp 2=Headlamp 3=Autolamp
int32_t CalcDayNightStatus(const uint8_t* pData)
{
  g_DayNightStatus = (pData[1] >> 6) & 0x03;
  return g_DayNightStatus;
}

void PrintDayNightStatus()
{
  const char* status = (g_DayNightStatus == 1) ? "Day" :
                       (g_DayNightStatus == 2) ? "Night" : "Unknown";
  Serial.printf("Day/Night Status = %d (%s)\n", g_DayNightStatus, status);
}

#endif  // _OBD2_CALCULATIONS
