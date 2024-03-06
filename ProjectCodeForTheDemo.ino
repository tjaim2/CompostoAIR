#include <Arduino.h>
#include <OneWire.h>
#include "FS.h"
#include <DallasTemperature.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <SoftwareSerial.h>

// Constants for TFT display color
#define TFT_BLACK 0x0000
#define TFT_WHITE 0xFFFF

// Pin definitions
#define temperature_sensor_pin 22
#define soil_moisture_pin 35
#define RE 25
#define DE 33

// Pin definitions
#define enablePin 27
#define motorPin1 14
#define motorPin2 12
#define spinButton 13

// Function prototypes
void spinIt();

/* More efficient delay*/
#define NOP __asm__ __volatile__ ("nop\n\t")

/* Debouncing */
unsigned long debounceDelay = 75;    // debounce time in ms


/* Adjust the motor timing and speed */
const int motorSpeed = 255;   /* [0 - 255] [off - fullspeed] */
const unsigned long spinTime = 2000;    /*  time Motor is spinning (in milliseconds) */
const unsigned long stopTime = 1000;    /* Time off between consecutive spins in same spin cycle (in milliseconds) */
const unsigned long clockAdjustment = 2; /* Ignore this: Used in SpinIt() function, minor adjustment to motor timing to compensate for instruction cycle limitations */

/* General structs for buttons */
struct Button {
  const int PIN;
  unsigned long timeLastPressed;
  bool pressed;
};

/* Spin button */
Button turnButton = {spinButton, 0, false};

/* handler for motor button */
void IRAM_ATTR isr() {
    unsigned long timePressed = millis();
    if (((timePressed - turnButton.timeLastPressed) > debounceDelay) && (turnButton.pressed == false)) {
      turnButton.pressed = true;
      turnButton.timeLastPressed = timePressed;      
    }
}

// OneWire and DallasTemperature setup for temperature sensor
OneWire oneWire(temperature_sensor_pin);
DallasTemperature sensors(&oneWire);

// TFT_eSPI setup for TFT display
TFT_eSPI tft = TFT_eSPI();

// Constants for sensor thresholds and status display
const int dry = 3060;
const int wet = 1200;
const int moistureMin = 1000;
const int moistureMax = 4000;
const int temperatureCMin = 32;
const int temperatureCMax = 70;
const int dryThreshold = 40;
const int wetThreshold = 65;
const int lowThreshold = 29;
float maxVal = 1999;  // Declare maxVal as a global variable

// Modbus RTU requests for reading NPK values
const byte nitro[] = {0x01, 0x03, 0x00, 0x1e, 0x00, 0x01, 0xe4, 0x0c};
const byte phos[] = {0x01, 0x03, 0x00, 0x1f, 0x00, 0x01, 0xb5, 0xcc};
const byte pota[] = {0x01, 0x03, 0x00, 0x20, 0x00, 0x01, 0x85, 0xc0};

byte values[11];

SoftwareSerial mod(26, 32); // (R0, DI)

// Function prototypes
void displayError(const char *errorMessage);

void displayData(float temperature, int sensorValue, byte val1, byte val2, byte val3);

void setup() {
  Serial.begin(9600);
  tft.init();
  tft.setRotation(1);
  sensors.begin();
  mod.begin(9600);
  pinMode(RE, OUTPUT);
  pinMode(DE, OUTPUT);
  delay(500);

   /* Configure Pins for motor control */
  pinMode(motorPin1, OUTPUT);     /* MotorPins Adjust Direction */
  pinMode(motorPin2, OUTPUT);
  pinMode(enablePin, OUTPUT);     /* Speed Adjustment */

  // Configure button for motor
  pinMode(turnButton.PIN, INPUT_PULLUP);
  attachInterrupt(turnButton.PIN, isr, FALLING);
  Serial.print("Interrupt cONFIGURED");
}

void loop() {
  // Temperature Reading
  sensors.requestTemperatures();
  float temperatureC = sensors.getTempCByIndex(0);
  Serial.print("Temperatures: ");
  Serial.print(temperatureC);
  Serial.println("C°");
 

  // Soil Moisture Reading
  int sensorValue = analogRead(soil_moisture_pin);
  delay(3000);
  

  // NPK Readings
  byte val1, val2, val3;
  val1 = nitrogen();
  delay(1500);
  val2 = phosphorous();
  delay(2550);
  val3 = potassium();
  delay(3600);

  // Print values over serial
 /*Serial.print("Nitrogen: ");
 Serial.println(val1);
 Serial.print("Phosphorous: ");
 Serial.println(val2);
 Serial.print("Potassium: ");
 Serial.println(val3);*/

  // Display data or error message
  if (temperatureC != DEVICE_DISCONNECTED_C && sensorValue >= moistureMin && sensorValue <= moistureMax) {
    displayData(temperatureC, sensorValue, val1, val2, val3);
  } else {
    displayError("Error reading data!");
  }

  delay(200);

  if (turnButton.pressed){
    spinIt();
    turnButton.pressed = false;
  }
  NOP;
}

void spinIt(){
  /* Update Status flag, Spin Cycle Start */

  /* set speed */
  analogWrite(enablePin, motorSpeed);

  /* reference time for delay implementation */
  unsigned long startTime = millis(); 

  /* Spin one direction */
  digitalWrite(motorPin1, HIGH);
  digitalWrite(motorPin2, LOW);
  unsigned long refTime = millis() - startTime + clockAdjustment;  
  while(refTime <= spinTime){
    refTime = millis() - startTime + clockAdjustment;
    Serial.print("refTime: "); Serial.println(refTime);
    Serial.print("spinTime: "); Serial.println(spinTime);
  }

  startTime = millis(); 
  
  /* Stop spinning */
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, LOW);
  refTime = millis() - startTime + clockAdjustment;
  Serial.print("refTime: "); Serial.println(refTime);
  while(refTime <= spinTime){
    refTime = millis() - startTime + clockAdjustment;
  }

  startTime = millis();
  
  /* Spin opposite direction */
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, HIGH);
  refTime = millis() - startTime + clockAdjustment;
  while(refTime <= spinTime){
    refTime = millis() - startTime + clockAdjustment;
  }
  startTime = millis();
    
  /* Stop spinning */
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, LOW);
  refTime = millis() - startTime + clockAdjustment;
  while(refTime <= spinTime){
    refTime = millis() - startTime + clockAdjustment;
  }

  analogWrite(enablePin, 0);
  }

void displayData(float temperature, int sensorValue, byte val1, byte val2, byte val3) {

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);

  // Display header
  tft.setCursor(10, 10, 2);
  tft.setRotation(1);
  tft.setTextSize(1.5);
  tft.print("Compost Air 360 Pro Max");
  tft.setTextSize(1);

  // Display temperature data
  tft.setCursor(10, 40, 2);
  tft.setRotation(1);
  tft.print("Temperature: ");
  tft.print(temperature);
  tft.println(" °C");

  // Display temperature status
  tft.setCursor(10, 60, 2);
  tft.setRotation(1);
  tft.print("Status: ");
  if (temperature <= (temperatureCMin - 5)) {  // Adjusted condition for "Low"
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.print("Low");
  } else if (temperature >= (temperatureCMax + 5)) {  // Adjusted condition for "High"
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.print("High");
  } else {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.print("Normal");
  }

  tft.setCursor(160, 40, 2);
  tft.setRotation(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print("Moisture: ");
  tft.print(sensorValue);
  tft.println();

  // Calculate and display soil moisture percentage
  int percentage = map(sensorValue, dry, wet, 0, 100);
  tft.setCursor(160, 60, 2);
  tft.setRotation(1);
  tft.print("Percentage: ");
  tft.print(percentage);
  tft.println("%");
  
  //for Serial.begin(9600) to show the moisture sensor,
  Serial.print("");
  Serial.print("Soil Moisture: ");
  Serial.print(percentage);
  Serial.println(" %");

  // Display soil moisture status
  tft.setCursor(160, 80, 2);
  tft.setRotation(1);
  tft.print("Status: ");
  if (percentage <= (dryThreshold - 5)) {  // Adjusted condition for "Low"
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.print("Low");
  } else if (percentage >= (wetThreshold + 5)) {  // Adjusted condition for "High"
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.print("High");
  } else {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.print("Normal");
  }

    // Display "Rotate Bin?" text
  tft.setCursor(10, 170, 2);
  tft.setRotation(1); // Adjust rotation value here
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print("Rotate Bin?");

  // Display "YES" text box
  tft.drawRect(100, 170, 40, 20, TFT_WHITE);
  tft.fillRect(100, 170, 40, 20, TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.setCursor(110, 170, 2); // Adjusted position and aligned text
  tft.print("YES");


  // Display NPK data
 tft.setCursor(10, 100, 2);
  tft.setRotation(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print("Nitrogen: ");
  float nitrogenPercentage = (val1 / maxVal) * 100;
  tft.print(nitrogenPercentage);
  tft.println(" %");

  //for Serial.begin(9600) to show the npk sensor,
 Serial.print("");
 Serial.print("Nitrogen: ");
 Serial.print(nitrogenPercentage);
 Serial.println(" %");

  // Adjusted condition for Nitrogen status level
  tft.setCursor(130, 105, 1);
  tft.setRotation(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print("Status: ");
  if (nitrogenPercentage >= (2.0 + 0.5)) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.print("High - Add browns");
  } else if (nitrogenPercentage <= (2.0 - 0.5)) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.print("Low - Add greens");
  } else {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.print("Normal");
  }

  // Display Phosphorous data with status
tft.setCursor(10, 120, 2); // Adjusted position
tft.setRotation(1);
tft.setTextColor(TFT_WHITE, TFT_BLACK);
tft.print("Phosphorous: ");
float phosphorousPercentage = (val2 / maxVal) * 100;
tft.print(phosphorousPercentage);
tft.println(" %");

//for Serial.begin(9600) to show the npk sensor,
Serial.print("");
Serial.print("Phosphorous: ");
Serial.print(phosphorousPercentage);
Serial.println(" %");

// Adjusted condition for Phosphorous status level
tft.setCursor(145, 125, 1); // Adjusted position
tft.setRotation(1);
tft.setTextColor(TFT_WHITE, TFT_BLACK);
tft.print("Status: ");
if (phosphorousPercentage >= 1.0 + 0.25) {
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.print("High-Add Browns");
} else if (phosphorousPercentage <= 0.5 - 0.25) {
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.print("Low-Add Greens"); //to increase phosp add manure, bone meal, 
} else {
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.print("Normal");
}

// Display Potassium data with status
tft.setCursor(10, 140, 2); // Adjusted position
tft.setRotation(1);
tft.setTextColor(TFT_WHITE, TFT_BLACK);
tft.print("Potassium: ");
float potassiumPercentage = (val3 / maxVal) * 100;
tft.print(potassiumPercentage);
tft.println(" %");

//for Serial.begin(9600) to show the npk sensor,
Serial.print("");
Serial.print("Potassium: ");
Serial.print(potassiumPercentage);
Serial.println(" %");


// Adjusted condition for Potassium status level
tft.setCursor(145, 145, 1); // Adjusted position
tft.setRotation(1);
tft.setTextColor(TFT_WHITE, TFT_BLACK);
tft.print("Status: ");
if (potassiumPercentage >= 2.0 + 0.5) {
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.print("High-Add water");
} else if (potassiumPercentage <= 2.0 - 0.5) {
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.print("Low-Add Browns");
} else {
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.print("Normal");
}
}

void displayError(const char *errorMessage) {
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(10, 10, 2);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.setTextSize(1);
  tft.println(errorMessage);

  Serial.print("");
  Serial.println(errorMessage);
}

byte nitrogen() {
  digitalWrite(DE, HIGH);
  digitalWrite(RE, HIGH);
  delay(10);
  if (mod.write(nitro, sizeof(nitro)) == 8) {
    digitalWrite(DE, LOW);
    digitalWrite(RE, LOW);
    for (byte i = 0; i < 7; i++) {
      values[i] = mod.read();
    }
    Serial.println();
  }
  return values[4];
}

byte phosphorous() {
  digitalWrite(DE, HIGH);
  digitalWrite(RE, HIGH);
  delay(20);
  if (mod.write(phos, sizeof(phos)) == 8) {
    digitalWrite(DE, LOW);
    digitalWrite(RE, LOW);
    for (byte i = 0; i < 7; i++) {
      values[i] = mod.read();
    }
    Serial.println();
  }
  return values[4];
}

byte potassium() {
  digitalWrite(DE, HIGH);
  digitalWrite(RE, HIGH);
  delay(30);
  if (mod.write(pota, sizeof(pota)) == 8) {
    digitalWrite(DE, LOW);
    digitalWrite(RE, LOW);
    for (byte i = 0; i < 7; i++) {
      values[i] = mod.read();
    }
    Serial.println();
  }
  return values[4];
}