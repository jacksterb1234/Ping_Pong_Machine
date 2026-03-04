#include <Arduino.h>

#define DECODE_NEC
#include <IRremote.hpp>

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// --- BLE ---
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// -------------------- PINS --------------------
const int IR_PIN = 5;

// A4988
const int STEP_PIN = 13;
const int DIR_PIN  = 12;
const int EN_PIN   = 27;
const bool A4988_ENABLE_ACTIVE_LOW = true;

// DRV8833
const int AIN1 = 32;
const int AIN2 = 33;
const int BIN1 = 26;
const int BIN2 = 25;

// LCD
const int LCD_ADDR = 0x27;
// Note: you had (LCD_ADDR, 22, 23) in your code, which is unusual for LiquidCrystal_I2C.
// If that worked for you, keep it. If not, typical call is (LCD_ADDR, 16, 2).
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

// DC Speed Setup
const uint32_t DC_PWM_FREQ = 20000;
const uint8_t  DC_PWM_RES  = 8;
uint8_t dc1Speed = 180;
uint8_t dc2Speed = 180;

// IR commands
const uint8_t IR_CMD_TOGGLE_STEPPER = 0x10;
const uint8_t IR_CMD_TOGGLE_DCS     = 0x11;
const uint8_t IR_CMD_SERVE_ONE      = 0x12;
const uint8_t IR_CMD_MOTOR1_SPDUP   = 0x13;
const uint8_t IR_CMD_MOTOR1_SLWDWN  = 0x14;
const uint8_t IR_CMD_MOTOR2_SPDUP   = 0x15;
const uint8_t IR_CMD_MOTOR2_SLWDWN  = 0x16;

// Stepper control
const int STEPS_PER_SERVE = 800;
const int STEP_DELAY_US   = 800;

// State
bool stepperEnabled = false;
bool dcMotorsOn     = false;
int  ballsServed    = 0;

// --- BLE UUIDs (match Expo app) ---
#define SERVICE_UUID   "0000FFFF-0000-1000-8000-00805F9B34FB"
#define CMD_CHAR_UUID  "0000FF01-0000-1000-8000-00805F9B34FB"

BLECharacteristic *cmdChar = nullptr;

// Continuous serve state
bool     continuousOn        = false;
uint32_t continuousIntervalMs = 400;
uint32_t lastServeMs         = 0;

// -------------------- LCD --------------------
void lcdUpdate() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Served: ");
  lcd.print(ballsServed);

  lcd.setCursor(0, 1);
  lcd.print("ST:");
  if (stepperEnabled) lcd.print("ON ");
  else                lcd.print("OFF");

  lcd.print(" DC:");
  if (dcMotorsOn) lcd.print("ON ");
  else            lcd.print("OFF ");

  lcd.print(dc1Speed / 255.0f * 100.0f, 0);
  lcd.print("%,");
  lcd.print(dc2Speed / 255.0f * 100.0f, 0);
  lcd.print("%");
}

// -------------------- Stepper --------------------
void setStepperEnabled(bool on) {
  stepperEnabled = on;
  pinMode(EN_PIN, OUTPUT);

  if (A4988_ENABLE_ACTIVE_LOW) {
    if (on) digitalWrite(EN_PIN, LOW);
    else    digitalWrite(EN_PIN, HIGH);
  } else {
    if (on) digitalWrite(EN_PIN, HIGH);
    else    digitalWrite(EN_PIN, LOW);
  }
}

void stepperMoveSteps(int steps, bool dir) {
  if (!stepperEnabled) return;

  digitalWrite(DIR_PIN, dir ? HIGH : LOW);

  for (int i = 0; i < steps; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(STEP_DELAY_US);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(STEP_DELAY_US);
  }
}

// -------------------- DC motors --------------------
void setDc1Speed() {
  digitalWrite(AIN2, LOW);
  analogWrite(AIN1, dc1Speed);
}

void setDc2Speed() {
  digitalWrite(BIN2, LOW);
  analogWrite(BIN1, dc2Speed);
}

void toggleDcMotors() {
  if (dcMotorsOn) {
    analogWrite(AIN1, 0);
    analogWrite(AIN2, 0);
    analogWrite(BIN1, 0);
    analogWrite(BIN2, 0);
  } else {
    setDc1Speed();
    setDc2Speed();
  }
  dcMotorsOn = !dcMotorsOn;
}

// -------------------- Serve --------------------
void serveOneBall() {
  if (!stepperEnabled) setStepperEnabled(true);
  stepperMoveSteps(STEPS_PER_SERVE, true);
  ballsServed++;
  lcdUpdate();
}

// -------------------- IR --------------------
void handleIrCommand(uint8_t cmd) {
  if (cmd == IR_CMD_TOGGLE_STEPPER) {
    setStepperEnabled(!stepperEnabled);
  } else if (cmd == IR_CMD_TOGGLE_DCS) {
    toggleDcMotors();
  } else if (cmd == IR_CMD_SERVE_ONE) {
    serveOneBall();
  } else if (cmd == IR_CMD_MOTOR1_SPDUP) {
    dc1Speed = min<uint8_t>(255, dc1Speed + 25);
    setDc1Speed();
  } else if (cmd == IR_CMD_MOTOR1_SLWDWN) {
    dc1Speed = max<uint8_t>(0, dc1Speed - 25);
    setDc1Speed();
  } else if (cmd == IR_CMD_MOTOR2_SPDUP) {
    dc2Speed = min<uint8_t>(255, dc2Speed + 25);
    setDc2Speed();
  } else if (cmd == IR_CMD_MOTOR2_SLWDWN) {
    dc2Speed = max<uint8_t>(0, dc2Speed - 25);
    setDc2Speed();
  }
  lcdUpdate();
}

// -------------------- BLE command handling --------------------
void handleBleCommand(const String &json) {
  Serial.print("BLE cmd: ");
  Serial.println(json);

  if (json.indexOf("\"type\":\"SERVE_ONE\"") >= 0) {
    serveOneBall();
  }
  else if (json.indexOf("\"type\":\"TOGGLE_DCS\"") >= 0) {
    toggleDcMotors();
    lcdUpdate();
  }
  else if (json.indexOf("\"type\":\"TOGGLE_STEPPER\"") >= 0) {
    setStepperEnabled(!stepperEnabled);
    lcdUpdate();
  }
  else if (json.indexOf("\"type\":\"MOTOR1_SPDUP\"") >= 0) {
    dc1Speed = min<uint8_t>(255, dc1Speed + 25);
    setDc1Speed();
    lcdUpdate();
  }
  else if (json.indexOf("\"type\":\"MOTOR1_SLWDWN\"") >= 0) {
    dc1Speed = max<uint8_t>(0, dc1Speed - 25);
    setDc1Speed();
    lcdUpdate();
  }
  else if (json.indexOf("\"type\":\"MOTOR2_SPDUP\"") >= 0) {
    dc2Speed = min<uint8_t>(255, dc2Speed + 25);
    setDc2Speed();
    lcdUpdate();
  }
  else if (json.indexOf("\"type\":\"MOTOR2_SLWDWN\"") >= 0) {
    dc2Speed = max<uint8_t>(0, dc2Speed - 25);
    setDc2Speed();
    lcdUpdate();
  }
  else if (json.indexOf("\"type\":\"START_CONTINUOUS\"") >= 0) {
    int idx = json.indexOf("\"intervalMs\":");
    if (idx >= 0) {
      int start = idx + String("\"intervalMs\":").length();
      int end = json.indexOf("}", start);
      if (end < 0) end = json.length();
      String num = json.substring(start, end);
      num.trim();
      uint32_t val = num.toInt();
      if (val >= 150 && val <= 3000) {
        continuousIntervalMs = val;
      }
    }
    continuousOn  = true;
    lastServeMs   = millis();
    Serial.println("Continuous mode: ON");
  }
  else if (json.indexOf("\"type\":\"STOP_CONTINUOUS\"") >= 0) {
    continuousOn = false;
    Serial.println("Continuous mode: OFF");
  }
  else {
    Serial.println("Unknown BLE command");
  }
}

class CommandCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *c) override {
    String value = c->getValue();
    if (value.isEmpty()) return;
    String json(value.c_str());
    handleBleCommand(json);
  }
};

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);

  // Stepper
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  setStepperEnabled(false);

  // DC pins
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);

  // DC PWM (adjust if your core uses ledcAttachPin style instead)
  analogWriteResolution(AIN1, DC_PWM_RES);
  analogWriteFrequency(AIN1, DC_PWM_FREQ);
  analogWriteResolution(BIN1, DC_PWM_RES);
  analogWriteFrequency(BIN1, DC_PWM_FREQ);

  digitalWrite(AIN2, LOW);
  digitalWrite(BIN2, LOW);
  analogWrite(AIN1, 0);
  analogWrite(BIN1, 0);

  // LCD
  lcd.init();
  lcd.backlight();
  lcdUpdate();

  // IR
  IrReceiver.begin(IR_PIN, DISABLE_LED_FEEDBACK);

  // BLE
  BLEDevice::init("PingPongRobot");
  BLEServer *server = BLEDevice::createServer();
  BLEService *service = server->createService(SERVICE_UUID);

  cmdChar = service->createCharacteristic(
    CMD_CHAR_UUID,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
  );
  cmdChar->setCallbacks(new CommandCallback());

  service->start();

  BLEAdvertising *adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(SERVICE_UUID);
  adv->setScanResponse(true);
  adv->setMinPreferred(0x06);
  adv->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("BLE advertising as PingPongRobot");
}

// -------------------- Loop --------------------
void loop() {
  // IR handling (unchanged)
  if (IrReceiver.decode()) {
    IrReceiver.printIRResultShort(&Serial);
    if ((IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT) == 0) {
      uint8_t cmd = IrReceiver.decodedIRData.command;
      handleIrCommand(cmd);
    }
    IrReceiver.resume();
  }

  // Continuous serve mode
  if (continuousOn) {
    uint32_t now = millis();
    if (now - lastServeMs >= continuousIntervalMs) {
      lastServeMs = now;
      serveOneBall();
    }
  }
}
