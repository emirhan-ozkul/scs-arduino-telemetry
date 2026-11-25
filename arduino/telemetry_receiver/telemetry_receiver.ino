/*
 * ETS2 / ATS Telemetry Serial Receiver
 *
 * Receives fixed 10-field packets from the Python application over USB Serial.
 * Serial settings: 115200 baud, newline-terminated packets.
 * Existing protocol is preserved:
 * T,1,speed_kmh,rpm,gear,fuel_l,remaining_km,odometer_km,
 * consumption_l_km,temperature_c
 *
 * This file contains no LCD, 7-segment, LED or button hardware code.
 * Add user hardware initialization and updates only in:
 *   setupUserHardware()
 *   updateUserHardware()
 */

#include <Arduino.h>
#include <stdint.h>

// Vehicle
float speedKmh = 0.0f;
int32_t gear = 0;
bool hasSpeed = false;
bool hasGear = false;

// Engine
float engineRpm = 0.0f;
float waterTemperatureC = 0.0f;
bool hasEngineRpm = false;
bool hasWaterTemperature = false;

// Fuel
float fuelLiters = 0.0f;
float fuelConsumptionLPerKm = 0.0f;
bool hasFuel = false;
bool hasFuelConsumption = false;

// Navigation
float remainingKm = 0.0f;
float odometerKm = 0.0f;
bool hasRemainingDistance = false;
bool hasOdometer = false;

// Connection and parser diagnostics
bool telemetryConnected = false;
unsigned long lastTelemetryMs = 0;
uint32_t invalidFrameCount = 0;

static char serialBuffer[160];
static uint8_t serialLength = 0;

static bool isNaField(const char* field)
{
  return field != nullptr && field[0] == 'N' && field[1] == 'A' && field[2] == '\0';
}

static bool parseFloatField(const char* field, float minimum, float maximum,
                            float& output)
{
  if (isNaField(field)) {
    return false;
  }

  bool digitSeen = false;
  bool decimalSeen = false;
  const char* cursor = field;

  if (*cursor == '-' || *cursor == '+') {
    ++cursor;
  }

  for (; *cursor != '\0'; ++cursor) {
    if (*cursor >= '0' && *cursor <= '9') {
      digitSeen = true;
    }
    else if (*cursor == '.' && !decimalSeen) {
      decimalSeen = true;
    }
    else {
      return false;
    }
  }

  if (!digitSeen) {
    return false;
  }

  float value = static_cast<float>(atof(field));
  if (value != value) {
    return false;
  }
  if (value < minimum || value > maximum) {
    return false;
  }

  output = value;
  return true;
}

static bool parseGearField(const char* field, int32_t& output)
{
  if (isNaField(field)) {
    return false;
  }

  char* end = nullptr;
  long value = strtol(field, &end, 10);
  if (end == field || *end != '\0') {
    return false;
  }
  if (value < -127L || value > 127L) {
    return false;
  }

  output = static_cast<int32_t>(value);
  return true;
}

static uint8_t splitFields(char* line, char* fields[10])
{
  uint8_t count = 0;
  char* start = line;

  for (char* cursor = line; ; ++cursor) {
    if (*cursor == ',' || *cursor == '\0') {
      if (count >= 10) {
        return 11;
      }

      if (*cursor == ',') {
        *cursor = '\0';
        fields[count++] = start;
        start = cursor + 1;
      }
      else {
        fields[count++] = start;
        return count;
      }
    }
  }
}

static bool fieldIsNa(const char* field)
{
  return isNaField(field);
}

static bool parseLiveFrame(char* fields[10])
{
  float parsedSpeed = 0.0f;
  float parsedRpm = 0.0f;
  int32_t parsedGear = 0;
  float parsedFuel = 0.0f;
  float parsedRemaining = 0.0f;
  float parsedOdometer = 0.0f;
  float parsedConsumption = 0.0f;
  float parsedTemperature = 0.0f;

  bool speedAvailable = !fieldIsNa(fields[2]);
  bool rpmAvailable = !fieldIsNa(fields[3]);
  bool gearAvailable = !fieldIsNa(fields[4]);
  bool fuelAvailable = !fieldIsNa(fields[5]);
  bool remainingAvailable = !fieldIsNa(fields[6]);
  bool odometerAvailable = !fieldIsNa(fields[7]);
  bool consumptionAvailable = !fieldIsNa(fields[8]);
  bool temperatureAvailable = !fieldIsNa(fields[9]);

  if (speedAvailable && !parseFloatField(fields[2], -100.0f, 100.0f, parsedSpeed)) return false;
  if (rpmAvailable && !parseFloatField(fields[3], 0.0f, 10000.0f, parsedRpm)) return false;
  if (gearAvailable && !parseGearField(fields[4], parsedGear)) return false;
  if (fuelAvailable && !parseFloatField(fields[5], 0.0f, 10000.0f, parsedFuel)) return false;
  if (remainingAvailable && !parseFloatField(fields[6], 0.0f, 1000000000.0f, parsedRemaining)) return false;
  if (odometerAvailable && !parseFloatField(fields[7], 0.0f, 1000000000.0f, parsedOdometer)) return false;
  if (consumptionAvailable && !parseFloatField(fields[8], 0.0f, 100.0f, parsedConsumption)) return false;
  if (temperatureAvailable && !parseFloatField(fields[9], -40.0f, 250.0f, parsedTemperature)) return false;

  // Commit globals only after every field in the complete frame is valid.
  if (speedAvailable) speedKmh = parsedSpeed;
  if (rpmAvailable) engineRpm = parsedRpm;
  if (gearAvailable) gear = parsedGear;
  if (fuelAvailable) fuelLiters = parsedFuel;
  if (remainingAvailable) remainingKm = parsedRemaining;
  if (odometerAvailable) odometerKm = parsedOdometer;
  if (consumptionAvailable) fuelConsumptionLPerKm = parsedConsumption;
  if (temperatureAvailable) waterTemperatureC = parsedTemperature;

  hasSpeed = speedAvailable;
  hasEngineRpm = rpmAvailable;
  hasGear = gearAvailable;
  hasFuel = fuelAvailable;
  hasRemainingDistance = remainingAvailable;
  hasOdometer = odometerAvailable;
  hasFuelConsumption = consumptionAvailable;
  hasWaterTemperature = temperatureAvailable;

  telemetryConnected = true;
  lastTelemetryMs = millis();
  return true;
}

static bool parseDisconnectedFrame(char* fields[10])
{
  for (uint8_t field = 2; field < 10; ++field) {
    if (!fieldIsNa(fields[field])) {
      return false;
    }
  }

  telemetryConnected = false;
  lastTelemetryMs = millis();

  hasSpeed = false;
  hasEngineRpm = false;
  hasGear = false;
  hasFuel = false;
  hasRemainingDistance = false;
  hasOdometer = false;
  hasFuelConsumption = false;
  hasWaterTemperature = false;

  return true;
}

static bool parseFrame(char* line)
{
  char* fields[10] = {nullptr};
  uint8_t fieldCount = splitFields(line, fields);

  if (fieldCount != 10) {
    ++invalidFrameCount;
    return false;
  }
  if (fields[0][0] != 'T' || fields[0][1] != '\0') {
    ++invalidFrameCount;
    return false;
  }

  if (fields[1][0] == '1' && fields[1][1] == '\0') {
    if (!parseLiveFrame(fields)) {
      ++invalidFrameCount;
      return false;
    }
    return true;
  }

  if (fields[1][0] == '0' && fields[1][1] == '\0') {
    if (!parseDisconnectedFrame(fields)) {
      ++invalidFrameCount;
      return false;
    }
    return true;
  }

  ++invalidFrameCount;
  return false;
}

static void receiveTelemetry()
{
  while (Serial.available() > 0) {
    char received = static_cast<char>(Serial.read());

    if (received == '\r') {
      continue;
    }

    if (received == '\n') {
      if (serialLength < sizeof(serialBuffer)) {
        serialBuffer[serialLength] = '\0';
        parseFrame(serialBuffer);
      }
      else {
        ++invalidFrameCount;
      }
      serialLength = 0;
      continue;
    }

    if (serialLength + 1 >= sizeof(serialBuffer)) {
      ++invalidFrameCount;
      serialLength = 0;
      continue;
    }

    serialBuffer[serialLength++] = received;
  }
}

static void updateConnectionStatus()
{
  if (telemetryConnected && lastTelemetryMs != 0) {
    unsigned long elapsed = millis() - lastTelemetryMs;
    if (elapsed >= 2500UL) {
      telemetryConnected = false;
      hasSpeed = false;
      hasEngineRpm = false;
      hasGear = false;
      hasFuel = false;
      hasRemainingDistance = false;
      hasOdometer = false;
      hasFuelConsumption = false;
      hasWaterTemperature = false;
    }
  }
}

void setupUserHardware()
{
  // Add the user's pinMode() and hardware initialization here.
  // No LCD, 7-segment, LED or button library is configured by default.
}

void updateUserHardware()
{
  // Add the user's LCD, 7-segment, LED and button code here.
  // The global telemetry variables and has... flags are ready for use.
}

void setup()
{
  Serial.begin(115200);
  setupUserHardware();
}

void loop()
{
  receiveTelemetry();
  updateConnectionStatus();
  updateUserHardware();
}
