// เพิ่มจอ LCD I2C 16x2 ที่อยู่ 0x27
// Arduino UNO ต่อ SDA -> A4, SCL -> A5
// หน้าจอใช้แถบแบต 4 ช่อง ช่องละประมาณ 25%
// ช่วง 0-24% ให้แถบเตือนกระพริบ
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

/*
==================================================
   วัดแบต LiFePO4 24V 45Ah
   Arduino UNO + วงจรแบ่งแรงดัน

   R1 = 56kΩ
   R2 = 10kΩ
   คาปาซิเตอร์แนะนำ 0.1uF, ตอนทดสอบใช้ 1.0uF
==================================================
*/

LiquidCrystal_I2C lcd(0x27, 16, 2); // สร้างจอ LCD I2C 16x2 ที่อยู่ 0x27

// -------------------------------------------------
// ตั้งค่าขา
// -------------------------------------------------
const byte BATTERY_PIN = A0; 

// -------------------------------------------------
// ตั้งค่าวงจรแบ่งแรงดัน
// -------------------------------------------------
const float R1 = 56000.00; // 56KΩ
const float R2 = 10000.00; // 10KΩ
//
// อัตราทดแรงดัน
const float DIVIDER_RATIO = (R1 + R2) / R2;

// -------------------------------------------------
// ตั้งค่า ADC
// -------------------------------------------------
const float ADC_REF = 5.0;
const int ADC_MAX   = 1023;

// -------------------------------------------------
// ค่าแรงดันอ้างอิงแบต
// -------------------------------------------------
const float VOLTAGE_FULL    = 26.65;      // V // วัดได้ตอนอะแดปเตอร์เป็นไฟเขียว
const float VOLTAGE_EMPTY   = 24.00;       // V // แสดง 0% และยังเหลือสำรองก่อน BMS ตัด
// const float MAX_CHARGE_CURRENT = 30.0;    //A // กระแสชาร์จสูงสุดของ TAICO
// const float MAX_DISCHARGE_CURRENT = 60.0; //A // กระแสจ่ายสูงสุดของ TAICO

// -------------------------------------------------
// ตั้งค่าการเฉลี่ย ADC
// -------------------------------------------------
const int SAMPLE_COUNT = 10;
const int SAMPLE_DELAY = 1;

// -------------------------------------------------
// ตั้งค่าจอ LCD
// -------------------------------------------------
const unsigned long LOW_BATTERY_BLINK_INTERVAL = 500; // กระพริบแถบเตือนทุก 500 ms
const byte LCD_FULL_BLOCK = 0; // เก็บรูปแถบเต็มในช่องอักขระพิเศษ 0
byte fullBlockChar[8] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
}; // วาดช่องเต็มขนาด 5x8 สำหรับแถบแบต

// -------------------------------------------------
// ตั้งค่ากันเปอร์เซ็นต์แกว่ง
// -------------------------------------------------
const int ADC_HYSTERESIS_COUNTS = 2; // ต้องเลยขอบ ADC ก่อนจึงเปลี่ยน %
const int PERCENT_CONFIRM_COUNT = 2; // ต้องอ่านทิศทางเดิมซ้ำก่อนเปลี่ยน 1%
const int PERCENT_SNAP_THRESHOLD = 5; // ต่างกันถึงค่านี้ให้ข้ามไปค่าใหม่ทันที

// -------------------------------------------------
// จุดเทียบค่า ADC ของแบต
// -------------------------------------------------
const int ADC_EMPTY = (int)(((VOLTAGE_EMPTY / DIVIDER_RATIO) * ADC_MAX / ADC_REF) + 0.5); // แปลงแรงดัน 0% เป็นค่า ADC
const int ADC_FULL  = (int)(((VOLTAGE_FULL / DIVIDER_RATIO) * ADC_MAX / ADC_REF) + 0.5); // แปลงแรงดันเต็มเป็นค่า ADC

// -------------------------------------------------
// ตัวแปรหลัก
// -------------------------------------------------
int adcValue = 0;

float a0Voltage          = 0.0;
float batteryVoltage     = 0.0;
float batteryPercent     = 0.0;
int stableBatteryPercent = 0;
bool stablePercentReady  = false; // เช็กว่าเริ่มค่าเปอร์เซ็นต์แล้วหรือยัง
int pendingPercentStep   = 0; // เก็บทิศทางที่รอเปลี่ยน: -1, 0, 1
int pendingStepRepeats   = 0; // นับว่าทิศทางเดิมซ้ำกี่ครั้ง

// int percentHistory[PERCENT_HISTORY_COUNT];
// int percentHistoryIndex  = 0;
// int percentHistoryCount  = 0;

// int lastDisplayPercent      = -1; // เก็บเปอร์เซ็นต์นิ่งล่าสุด
// int possibleErrorPercent    = -1; // เก็บค่าที่อาจเป็นการเด้ง
// int possibleErrorRepeatCount = 0; // นับค่าซ้ำเพื่อยืนยัน
// unsigned long displayErrors = 0; // นับการเด้ง เช่น 18 -> 19 -> 18

String batteryStatus = "";

// =================================================
// เริ่มระบบ
// =================================================
void setup() {

  Serial.begin(9600);

  lcd.init(); // เริ่มจอ LCD ก่อนพิมพ์ข้อความ
  lcd.createChar(LCD_FULL_BLOCK, fullBlockChar); // ลงทะเบียนรูปแถบเต็มไว้ใช้แสดงแบต
  lcd.backlight(); // เปิดไฟหลังจอไว้ตลอดการทำงาน
  lcd.setCursor(0, 0); // ตั้งตำแหน่งข้อความเริ่มต้น
  lcd.print(F("System Initializing"));
  delay(1500);
  lcd.clear(); // ล้างข้อความเริ่มต้นก่อนแสดงค่าแบตจริง

  pinMode(BATTERY_PIN, INPUT);
}

// =================================================
// วนอ่านค่า
// =================================================
void loop() {

  // อ่านค่า ADC แบบเฉลี่ย
  adcValue = readAverageADC(BATTERY_PIN);

  // แปลง ADC เป็นแรงดันที่ A0
  a0Voltage = adcToVoltage(adcValue);

  // คำนวณแรงดันแบตจริง
  batteryVoltage = a0Voltage * DIVIDER_RATIO;

  // คำนวณเปอร์เซ็นต์แบต
  stableBatteryPercent = calculateStableBatteryPercentFromADC(adcValue); // อัปเดต % แบบกันแกว่งจาก ADC
  batteryPercent = stableBatteryPercent; // ใช้ค่าเต็มจำนวนที่ผ่านการกรองแล้ว
  // handleDisplayPercentError(stableBatteryPercent); // เช็กเปอร์เซ็นต์เด้งกลับ

  // อ่านสถานะแบต
  // batteryStatus = getBatteryStatus(batteryVoltage);

  // แสดงข้อมูลทั้งหมด
  displayBatteryData(); // ใช้ delay ร่วมกับ Serial Monitor

  delay(600);
}

// =================================================
// อ่านค่า ADC เฉลี่ย
// =================================================
int readAverageADC(int pin) {

  long total = 0;

  for (int i = 0; i < SAMPLE_COUNT; i++) {

    total += analogRead(pin);

    delay(SAMPLE_DELAY);
  }

  return total / SAMPLE_COUNT;
}

// แปลง ADC เป็นแรงดัน
// =================================================
// แปลงค่า ADC ดิบเป็นโวลต์
float adcToVoltage(int adc) { return (adc * ADC_REF) / ADC_MAX; }

// =================================================
// คำนวณเปอร์เซ็นต์แบต
// =================================================
// ใช้สูตรเส้นตรงแบบประมาณค่า
float calculateBatteryPercent(float voltage) {

  float percent =
      ((voltage - VOLTAGE_EMPTY) / (VOLTAGE_FULL - VOLTAGE_EMPTY)) * 100.0;

  // จำกัดค่าให้อยู่ระหว่าง 0-100%
  percent = constrain(percent, 0.0, 100.0);

  return percent;
}

// =================================================
// คำนวณเปอร์เซ็นต์ดิบจาก ADC
// =================================================
int calculateRawPercentFromADC(int adc) {

  long adcRange = ADC_FULL - ADC_EMPTY; // ช่วง ADC ที่ใช้ระหว่างว่างกับเต็ม

  if (adcRange <= 0) {
    return 0; // กันค่าเทียบผิดถ้าจุดเต็มต่ำกว่าจุดว่าง
  }

  long adcOffset = adc - ADC_EMPTY; // ระยะ ADC จากจุด 0%
  long percent = ((adcOffset * 100L) + (adcRange / 2)) / adcRange; // แปลงเป็นเปอร์เซ็นต์จำนวนเต็ม

  return constrain((int)percent, 0, 100); // จำกัดค่าให้อยู่ในช่วงที่แสดงได้
}

// =================================================
// คำนวณขอบ ADC ของแต่ละเปอร์เซ็นต์
// =================================================
int calculatePercentEdgeADC(int percent) {

  long adcRange = ADC_FULL - ADC_EMPTY; // ช่วง ADC ที่ใช้ระหว่างว่างกับเต็ม
  long adcEdge = ADC_EMPTY + ((adcRange * (long)percent) / 100L); // แปลงขอบ % เป็นค่า ADC

  return (int)adcEdge; // คืนค่า ADC ของขอบนี้
}

// =================================================
// คำนวณเปอร์เซ็นต์นิ่งจาก ADC
// =================================================
int calculateStableBatteryPercentFromADC(int adc) {

  int rawPercent = calculateRawPercentFromADC(adc); // คำนวณ % ดิบก่อนกรอง

  if (!stablePercentReady) {
    stableBatteryPercent = rawPercent; // ตั้งค่าเริ่มต้นจาก ADC ครั้งแรก
    stablePercentReady = true; // บันทึกว่าเริ่มค่าแล้ว
    return stableBatteryPercent;
  }

  int percentGap = abs(rawPercent - stableBatteryPercent); // ดูว่าค่าดิบห่างจากค่าที่แสดงเท่าไร

  if (percentGap >= PERCENT_SNAP_THRESHOLD) {
    stableBatteryPercent = rawPercent; // ข้ามไปค่าใหม่เมื่อเปลี่ยนจริง
    pendingPercentStep = 0; // ล้างทิศทางที่รออยู่
    pendingStepRepeats = 0; // ล้างตัวนับยืนยัน
    return stableBatteryPercent;
  }

  int requestedStep = 0; // ค่าเริ่มต้นคือยังไม่เปลี่ยน %

  if (stableBatteryPercent < 100) {
    int upEdgeADC = calculatePercentEdgeADC(stableBatteryPercent + 1); // หา ADC ขอบเปอร์เซ็นต์ถัดไป

    if (adc >= upEdgeADC + ADC_HYSTERESIS_COUNTS) {
      requestedStep = 1; // ขอเพิ่ม 1% เมื่อ ADC เลยขอบบนแล้ว
    }
  }

  if (stableBatteryPercent > 0) {
    int downEdgeADC = calculatePercentEdgeADC(stableBatteryPercent); // หา ADC ขอบล่างของ % ปัจจุบัน

    if (adc <= downEdgeADC - ADC_HYSTERESIS_COUNTS) {
      requestedStep = -1; // ขอลด 1% เมื่อ ADC เลยขอบล่างแล้ว
    }
  }

  if (requestedStep == 0) {
    pendingPercentStep = 0; // ล้างทิศทางเมื่อ ADC กลับมาในช่วงนิ่ง
    pendingStepRepeats = 0; // ล้างตัวนับเมื่อยังไม่เปลี่ยนค่า
    return stableBatteryPercent;
  }

  if (requestedStep == pendingPercentStep) {
    pendingStepRepeats++; // นับซ้ำเมื่อทิศทางเดิมยังมาอีก
  } else {
    pendingPercentStep = requestedStep; // เริ่มตามทิศทางใหม่
    pendingStepRepeats = 1; // นับครั้งแรกของทิศทางใหม่
  }

  if (pendingStepRepeats >= PERCENT_CONFIRM_COUNT) {
    stableBatteryPercent += requestedStep; // เปลี่ยนค่าที่แสดงทีละ 1%
    stableBatteryPercent = constrain(stableBatteryPercent, 0, 100); // จำกัดค่า 0-100%
    pendingPercentStep = 0; // ล้างทิศทางหลังยอมรับค่า
    pendingStepRepeats = 0; // ล้างตัวนับหลังยอมรับค่า
  }

  return stableBatteryPercent;
}

// =================================================
// โค้ดเก่า: เก็บประวัติเปอร์เซ็นต์
// =================================================
// void addBatteryPercentReading(int percent) {

//   percentHistory[percentHistoryIndex] = percent;
//   percentHistoryIndex = (percentHistoryIndex + 1) % PERCENT_HISTORY_COUNT;

//   if (percentHistoryCount < PERCENT_HISTORY_COUNT) {
//     percentHistoryCount++;
//   }
// }

// =================================================
// โค้ดเก่า: หาเปอร์เซ็นต์ที่นิ่ง
// =================================================
// int calculateStableBatteryPercent() {

//   int bestPercent = percentHistory[0];
//   int bestCount = 0;

//   for (int i = 0; i < percentHistoryCount; i++) {

//     int historyIndex =
//         (percentHistoryIndex - percentHistoryCount + i + PERCENT_HISTORY_COUNT) %
//         PERCENT_HISTORY_COUNT;
//     int candidatePercent = percentHistory[historyIndex];
//     int candidateCount = 0;

//     for (int j = 0; j < percentHistoryCount; j++) {

//       if (percentHistory[j] == candidatePercent) {
//         candidateCount++;
//       }
//     }

//     if (candidateCount >= bestCount) {
//       bestPercent = candidatePercent;
//       bestCount = candidateCount;
//     }
//   }

//   return bestPercent;
// }

// =================================================
// โค้ดทดลอง: จับเปอร์เซ็นต์เด้ง
// =================================================
// void handleDisplayPercentError(int currentPercent) {

//   if (lastDisplayPercent < 0) {
//     lastDisplayPercent = currentPercent; // ตั้งค่าแรกที่ยอมรับ
//     return;
//   }

//   if (possibleErrorPercent >= 0) {

//     if (currentPercent == lastDisplayPercent) {
//       displayErrors++; // นับการเด้งหนึ่งครั้ง
//       possibleErrorPercent = -1; // ล้างค่าที่สงสัยว่าเด้ง
//       possibleErrorRepeatCount = 0; // ล้างตัวนับซ้ำ
//       return;
//     }

//     if (currentPercent == possibleErrorPercent) {
//       possibleErrorRepeatCount++; // ยืนยันว่าค่าใหม่ซ้ำจริง
//     } else {
//       lastDisplayPercent = currentPercent; // ยอมรับค่าใหม่เป็นค่านิ่งล่าสุด
//       possibleErrorPercent = -1; // ล้างค่าที่สงสัยเดิม
//       possibleErrorRepeatCount = 0; // ล้างตัวนับซ้ำ
//       return;
//     }

//     if (possibleErrorRepeatCount >= 2) {
//       lastDisplayPercent = currentPercent; // ยอมรับค่าใหม่หลังอ่านซ้ำ
//       possibleErrorPercent = -1; // ล้างค่าที่รับแล้ว
//       possibleErrorRepeatCount = 0; // ล้างตัวนับซ้ำ
//     }

//     return;
//   }

//   if (currentPercent != lastDisplayPercent) {
//     possibleErrorPercent = currentPercent; // เก็บค่าที่อาจเด้งครั้งเดียว
//     possibleErrorRepeatCount = 1; // เริ่มนับค่าซ้ำ
//   }
// }

// =================================================
// โค้ดเก่า: อ่านสถานะแบต
// =================================================
// เช็กสถานะจากแรงดัน
// String getBatteryStatus(float voltage) {

//   if (voltage > VOLTAGE_OVER) {
//     return "OVER VOLTAGE";
//   }

//   if (voltage <= VOLTAGE_EMPTY) {
//     return "CRITICAL LOW";
//   }

//   if (voltage < VOLTAGE_WARNING) {
//     return "WARNING LOW";
//   }

//   if (voltage < VOLTAGE_LOW) {
//     return "LOW BATTERY";
//   }

//   return "NORMAL";
// }
// =================================================
// แสดงผลผ่าน Serial 9600
// =================================================
void printBatteryBarCells(byte filledCells) {

  for (byte i = 0; i < 4; i++) {
    if (i < filledCells) {
      lcd.write(LCD_FULL_BLOCK); // วาดช่องแบตที่เต็ม
    } else {
      lcd.print(F(" ")); // เว้นช่องว่างเพื่อล้างรูปเก่า
    }
  }
}

void printBatteryBar(int percent) {

  if (percent < 25) {
    bool showWarningBar = ((millis() / LOW_BATTERY_BLINK_INTERVAL) % 2) == 0; // กระพริบเตือนช่วง 0-24%

    printBatteryBarCells(showWarningBar ? 1 : 0);
    return;
  }

  if (percent < 50) {
    printBatteryBarCells(1); // แสดง 1 ช่องเมื่อ 25-49%
    return;
  }

  if (percent < 75) {
    printBatteryBarCells(2); // แสดง 2 ช่องเมื่อ 50-74%
    return;
  }

  if (percent < 100) {
    printBatteryBarCells(3); // แสดง 3 ช่องเมื่อ 75-99%
    return;
  }

  printBatteryBarCells(4); // แสดง 4 ช่องเมื่อ 100%
}

void displayBatteryLCD() {

  lcd.setCursor(0, 0); // บรรทัดแรกแสดงแบตแบบ AGV
  lcd.print(F("Battery: "));
  printBatteryBar(stableBatteryPercent);
  lcd.print(F("   ")); // ล้างตัวอักษรค้างหลังแถบแบต

  lcd.setCursor(0, 1); // บรรทัดสองแสดงสถานะคงที่
  lcd.print(F("Status: Monitor "));
}

void displayBatteryData() {

  // Serial.println(F("================================"));
  // Serial.println(F("      LiFePO4 Battery Monitor"));
  // Serial.println(F("================================"));

  // Serial.print(F("ADC Value       : "));
  // Serial.println(adcValue);

  // Serial.print(F("A0 Voltage      : "));
  // Serial.print(a0Voltage, 2);
  // Serial.println(F(" V"));

  Serial.print(F("Battery Voltage : "));
  Serial.print(batteryVoltage, 2);
  Serial.println(F(" V"));

  Serial.print(F("Battery Percent : "));
  Serial.print(stableBatteryPercent); // พิมพ์ % ที่ผ่านการกรองแล้ว
  Serial.println(F(" %"));

  displayBatteryLCD(); // อัปเดตแถบแบตบน LCD

  // Serial.print(F("Errors  : "));
  // Serial.println(displayErrors); // พิมพ์จำนวนครั้งที่ % เด้ง

  Serial.println();
}
