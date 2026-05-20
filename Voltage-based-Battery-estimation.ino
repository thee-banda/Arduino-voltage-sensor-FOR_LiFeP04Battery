/*
==================================================
   LiFePO4 Battery Monitor (24V 45Ah)
   Arduino UNO + Voltage Divider

   R1 = 56kΩ
   R2 = 10kΩ
==================================================
*/

// -------------------------------------------------
// PIN CONFIGURATION
// -------------------------------------------------
const byte BATTERY_PIN = A0;

// -------------------------------------------------
// VOLTAGE DIVIDER CONFIGURATION
// -------------------------------------------------
const float R1 = 56000.00; // 56KΩ
const float R2 = 10000.00; // 10KΩ

// Voltage divider ratio
const float DIVIDER_RATIO = (R1 + R2) / R2;

// -------------------------------------------------
// ADC CONFIGURATION
// -------------------------------------------------
const float ADC_REF = 5.0;
const int ADC_MAX = 1023;

// -------------------------------------------------
// BATTERY VOLTAGE THRESHOLDS
// -------------------------------------------------
const float VOLTAGE_FULL = 30;      // V
const float   VOLTAGE_LOW = 25.6;     // V
const float VOLTAGE_WARNING = 24.8; // V
const float VOLTAGE_EMPTY =
    24.0; // V  // Fixed typo (removed stray period) and added comment
const float VOLTAGE_OVER = 29.2; // V

// -------------------------------------------------
// ADC AVERAGING SETTINGS
// -------------------------------------------------
const int SAMPLE_COUNT = 10;
const int SAMPLE_DELAY = 1;

// -------------------------------------------------
// BATTERY PERCENT SMOOTHING SETTINGS
// -------------------------------------------------
const int PERCENT_HISTORY_COUNT = 10;

// -------------------------------------------------
// GLOBAL VARIABLES
// -------------------------------------------------
int adcValue = 0;

float a0Voltage = 0.0;
float batteryVoltage = 0.0;
float batteryPercent = 0.0;
int stableBatteryPercent = 0;

int percentHistory[PERCENT_HISTORY_COUNT];
int percentHistoryIndex = 0;
int percentHistoryCount = 0;

String batteryStatus = "";

// =================================================
// SETUP
// =================================================
void setup() {

  Serial.begin(9600);

  pinMode(BATTERY_PIN, INPUT);
}

// =================================================
// MAIN LOOP
// =================================================
void loop() {

  // Read averaged ADC value
  adcValue = readAverageADC(BATTERY_PIN);

  // Convert ADC value to A0 voltage
  a0Voltage = adcToVoltage(adcValue);

  // Calculate actual battery voltage
  batteryVoltage = a0Voltage * DIVIDER_RATIO;

  // Calculate battery percentage
  batteryPercent = calculateBatteryPercent(batteryVoltage);
  addBatteryPercentReading((int)(batteryPercent + 0.5));
  stableBatteryPercent = calculateStableBatteryPercent();

  // Get battery status
  batteryStatus = getBatteryStatus(batteryVoltage);

  // Display all data
  displayBatteryData();

  delay(1000);
}

// =================================================
// READ AVERAGED ADC VALUE
// =================================================
int readAverageADC(int pin) {

  long total = 0;

  for (int i = 0; i < SAMPLE_COUNT; i++) {

    total += analogRead(pin);

    delay(SAMPLE_DELAY);
  }

  return total / SAMPLE_COUNT;
}

// =================================================
// CONVERT ADC VALUE TO VOLTAGE
// =================================================
// CONVERT ADC VALUE TO VOLTAGE
// =================================================
// Convert raw ADC reading to voltage (in volts)
float adcToVoltage(int adc) { return (adc * ADC_REF) / ADC_MAX; }

// =================================================
// CALCULATE BATTERY PERCENTAGE
// =================================================
// CALCULATE BATTERY PERCENTAGE
// =================================================
// Compute state‑of‑charge percentage (linear approximation)
float calculateBatteryPercent(float voltage) {

  float percent =
      ((voltage - VOLTAGE_EMPTY) / (VOLTAGE_FULL - VOLTAGE_EMPTY)) * 100.0;

  // Limit value between 0% and 100%
  percent = constrain(percent, 0.0, 100.0);

  return percent;
}

// =================================================
// ADD BATTERY PERCENT READING TO HISTORY
// =================================================
void addBatteryPercentReading(int percent) {

  percentHistory[percentHistoryIndex] = percent;
  percentHistoryIndex = (percentHistoryIndex + 1) % PERCENT_HISTORY_COUNT;

  if (percentHistoryCount < PERCENT_HISTORY_COUNT) {
    percentHistoryCount++;
  }
}

// =================================================
// CALCULATE STABLE BATTERY PERCENTAGE
// =================================================
int calculateStableBatteryPercent() {

  int bestPercent = percentHistory[0];
  int bestCount = 0;

  for (int i = 0; i < percentHistoryCount; i++) {

    int historyIndex =
        (percentHistoryIndex - percentHistoryCount + i + PERCENT_HISTORY_COUNT) %
        PERCENT_HISTORY_COUNT;
    int candidatePercent = percentHistory[historyIndex];
    int candidateCount = 0;

    for (int j = 0; j < percentHistoryCount; j++) {

      if (percentHistory[j] == candidatePercent) {
        candidateCount++;
      }
    }

    if (candidateCount >= bestCount) {
      bestPercent = candidatePercent;
      bestCount = candidateCount;
    }
  }

  return bestPercent;
}

// =================================================
// GET BATTERY STATUS
// =================================================
// GET BATTERY STATUS
// =================================================
// Determine battery condition based on voltage thresholds
String getBatteryStatus(float voltage) {

  if (voltage > VOLTAGE_OVER) {
    return "OVER VOLTAGE";
  }

  if (voltage <= VOLTAGE_EMPTY) {
    return "CRITICAL LOW";
  }

  if (voltage < VOLTAGE_WARNING) {
    return "WARNING LOW";
  }

  if (voltage < VOLTAGE_LOW) {
    return "LOW BATTERY";
  }

  return "NORMAL";
}

// Serial 9600 OUTPUT:
void displayBatteryData() {

/* Serial.println(F("================================"));
  Serial.println(F("      LiFePO4 Battery Monitor"));
  Serial.println(F("================================"));

  Serial.print(F("ADC Value       : "));
  Serial.println(adcValue);

  Serial.print(F("A0 Voltage      : "));
  Serial.print(a0Voltage, 2);
  Serial.println(F(" V"));

  Serial.print(F("Battery Voltage : "));
  Serial.print(batteryVoltage, 2);
  Serial.println(F(" V"));
*/
  Serial.print(F("Battery Percent : "));
  Serial.print(stableBatteryPercent);
  Serial.println(F(" %"));

  Serial.println();
}
