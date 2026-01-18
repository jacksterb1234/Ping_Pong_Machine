
#include <Arduino.h>

#define DECODE_NEC
#include <IRremote.hpp> // IrReceiver.decode(), IrReceiver.decodedIRData, IrReceiver.resume() [web:4]

#include <Wire.h>
#include <LiquidCrystal_I2C.h> // lcd.init(), lcd.backlight() [web:51]

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
LiquidCrystal_I2C lcd(LCD_ADDR, 22, 23);

// DC Speed Setup
const uint32_t DC_PWM_FREQ = 20000; 
const uint8_t  DC_PWM_RES  = 8;  
uint8_t dc1Speed = 180;        
uint8_t dc2Speed = 180;

// IR commands (need to change addresses when have)
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

// LCD
void lcdUpdate() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Served: ");
  lcd.print(ballsServed);

  lcd.setCursor(0, 1);
  lcd.print("ST:");
  if (stepperEnabled) lcd.print("ON ");
  else               lcd.print("OFF");

  lcd.print(" DC:");
  if (dcMotorsOn) lcd.print("ON ");
  else           lcd.print("OFF ");
  lcd.print(dc1Speed / 255);
  lcd.print("%, ");
  lcd.print(dc2Speed / 255);
  lcd.print("%");
}

// Stepper
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

  if (dir) digitalWrite(DIR_PIN, HIGH);
  else     digitalWrite(DIR_PIN, LOW);

  for (int i = 0; i < steps; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(STEP_DELAY_US);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(STEP_DELAY_US);
  }
}

// DC motors
void setDc1Speed() {
  digitalWrite(AIN2, LOW);

  analogWrite(AIN1, dc1Speed);
}
void setDc2Speed() {
  digitalWrite(AIN2, LOW);

  analogWrite(AIN1, dc2Speed);

}

void toggleDcMotors() {
  if (dcMotorsOn) {
    analogWrite(AIN1, 0);
    analogWrite(AIN2, 0);
  }
  else {
    setDc1Speed();
    setDc2Speed();
  }
  dcMotorsOn = !dcMotorsOn;
}

// Serve
void serveOneBall() {
  if (!stepperEnabled) setStepperEnabled(true);

  stepperMoveSteps(STEPS_PER_SERVE, true);
  ballsServed++;
  lcdUpdate();
}

// IR 
void handleIrCommand(uint8_t cmd) {
  if (cmd == IR_CMD_TOGGLE_STEPPER) {
    if (stepperEnabled) setStepperEnabled(false);
    else               setStepperEnabled(true);
    return;
  }

  if (cmd == IR_CMD_TOGGLE_DCS) {
    toggleDcMotors();
  }

  if (cmd == IR_CMD_SERVE_ONE) {
    serveOneBall();
  }
  if (cmd == IR_CMD_MOTOR1_SPDUP) {
    dc1Speed += 25;
    setDc1Speed();
  }
  if (cmd == IR_CMD_MOTOR1_SLWDWN) {
    dc1Speed -= 25;
    setDc1Speed();
  }
  if (cmd == IR_CMD_MOTOR2_SPDUP) {
    dc2Speed += 25;
    setDc2Speed();
  }
  if (cmd == IR_CMD_MOTOR2_SLWDWN) {
    dc2Speed -= 25;
    setDc2Speed();
  }
lcdUpdate();
}

// ESP32 Setup
void setup() {
  Serial.begin(115200);

  // Stepper Pins
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  setStepperEnabled(false);

  // DC pins
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);

  // DC PMW
  analogWriteResolution(AIN1, DC_PWM_RES);
  analogWriteFrequency(AIN1, DC_PWM_FREQ);
  analogWriteResolution(BIN1, DC_PWM_RES);
  analogWriteFrequency(BIN1, DC_PWM_FREQ);

  digitalWrite(AIN2, LOW);
  digitalWrite(BIN2, LOW);
  analogWrite(AIN1, 0);
  analogWrite(BIN1, 0);

  // LCD setup
  lcd.init();
  lcd.backlight();
  lcdUpdate();

  // IR setup
  IrReceiver.begin(IR_PIN, DISABLE_LED_FEEDBACK);
}

void loop() {
  if (IrReceiver.decode()) {
    IrReceiver.printIRResultShort(&Serial); 

    if ((IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT) == 0) {
      uint8_t cmd = IrReceiver.decodedIRData.command; 
      handleIrCommand(cmd);
    }

    IrReceiver.resume(); 
  }
}
