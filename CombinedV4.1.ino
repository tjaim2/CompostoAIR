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

/* More efficient delay*/
#define NOP __asm__ __volatile__ ("nop\n\t")

// Pin definitions
#define temperature_sensor_pin 22
#define soil_moisture_pin 35
#define RE 25
#define DE 33
#define enablePin 27
#define motorPin1 14
#define motorPin2 12
#define spinButton 13

// OneWire and DallasTemperature setup for temperature sensor
OneWire oneWire(temperature_sensor_pin);
DallasTemperature sensors(&oneWire);

// Function prototypes
void displayError(const char *errorMessage);
void displayData(float temperature, int sensorValue, byte val1, byte val2, byte val3);
void spinIt();
void yonDelay();

// TFT_eSPI setup for TFT display
TFT_eSPI tft = TFT_eSPI();

// Constants for sensor thresholds and status display
const int dry = 2830;
const int wet = 1030;
const int moistureMin = 300;
const int moistureMax = 3000;
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

SoftwareSerial mod(26, 32);

// Define variables to be used for sleep
#define us_from_seconds 1000000  /*micro seconds to seconds */
#define us_from_mins 60000000  /* seconds to minutes*/
#define us_from_hours 3600000000    /*us to hours*/
#define sleep_time  5        /* ESP32 will go to sleep for this long. Specify units by multiplying by the desired "us_from_XXXX" when defining "time_in_us" below (Ex: if you want units to be seconds, multiply sleep_time * us_from seconds) */
unsigned long time_in_us = sleep_time * us_from_seconds;  /* use for esp_deep_sleep function */
unsigned long timeUntilSleep = 10000; /* How long without an input until system goes to sleep (in ms) */
unsigned long autoSpinTime = 100000; /* How long between automatic turns (in ms) */
unsigned long lastInput;              /* Tracks when the last user input was (in ms)*/
unsigned long setUpTime;              /* Tracks time setup function (in ms)*/
unsigned long loopTime;               /* Tracks how long each loop is (in ms) */
unsigned long refTime;                /* General use time indexing variable (in ms) */
unsigned long startTime;              /* Time indexing variable (in ms) */
RTC_DATA_ATTR unsigned long testTime;
RTC_DATA_ATTR unsigned long lastSpin = 0;
RTC_DATA_ATTR unsigned long rebootCount = 0;
bool spun = false;


/* Variables That Adjust motor timing and speed */
const int motorSpeed = 255;   /* [0 - 255] [off - fullspeed] */
const unsigned long spinTime = 2000;    /*  time Motor is spinning (in milliseconds) */
const unsigned long stopTime = 1000;    /* Time off between consecutive spins in same spin cycle (in milliseconds) */
const unsigned long clockAdjustment = 1; /* Ignore this: Used in SpinIt() function, minor adjustment to motor timing to compensate for instruction cycle limitations */

/* General structs for button */
struct Button {
  const int PIN;
  unsigned long timeLastPressed;
  bool pressed;
};

/* Spin button */
Button turnButton = {spinButton, 0, false};

/* Debouncing */
unsigned long debounceDelay = 75;    // debounce time in ms

void IRAM_ATTR isr() {
    unsigned long timePressed = millis();
    if (((timePressed - turnButton.timeLastPressed) > debounceDelay) && (turnButton.pressed == false)) {
      turnButton.pressed = true;
      turnButton.timeLastPressed = timePressed;
      lastInput = millis();
      lastSpin = lastInput;
      spun = true;
    }
}

void setup() {
  unsigned long rebootTime = sleep_time * us_from_seconds / 1000;       //sleep_time assumed to be in seconds
  if (rebootCount != 0){
//    unsigned long rebootTime = sleep_time * 1000;
    lastSpin = lastSpin + rebootTime;      
  }
  rebootCount++;
  lastInput = millis();
  
  /* debugging
  Serial.begin(115200);
  delay(10);
  Serial.print("lastSpin:");
  Serial.println(lastSpin);
  delay(10);
  Serial.print("rebootTime:");
  Serial.println(rebootTime);
  delay(10);
  end debugging */
  
  Serial.begin(9600);
  tft.init();
  tft.setRotation(1);
  sensors.begin();
  mod.begin(9600);
  pinMode(RE, OUTPUT);
  pinMode(DE, OUTPUT);
  yonDelay(500);     //delay(500);
  
  /* Configure Pins for motor control */
  pinMode(motorPin1, OUTPUT);     /* MotorPins Adjust Direction */
  pinMode(motorPin2, OUTPUT);
  pinMode(enablePin, OUTPUT);     /* Speed Adjustment */

  /* Configure motor activation on button push */
  pinMode(turnButton.PIN, INPUT_PULLUP);
  attachInterrupt(turnButton.PIN, isr, FALLING);

  setUpTime = millis();
  lastSpin = lastSpin + setUpTime;
  
  /*Serial.begin(115200);
  delay(2);
  Serial.print("setUpTime:");
  Serial.println(setUpTime);
  Serial.flush();
  Serial.begin(9600);*/
}

void loop() {
  loopTime = millis();

  // Reset button flag
  if (turnButton.pressed) {
    spinIt();
    turnButton.pressed = false;
  }
  
    // Temperature Reading
  sensors.requestTemperatures();
  float temperatureC = sensors.getTempCByIndex(0);

  // Soil Moisture Reading
  int sensorValue = analogRead(soil_moisture_pin);

  // NPK Readings
  byte val1, val2, val3;
  
  val1 = nitrogen();
  yonDelay(250);    //delay(250);
  
  val2 = phosphorous();
  yonDelay(250);    //delay(250);
  
  val3 = potassium();
  yonDelay(250);    //delay(250);

  // Display data or error message
  if (temperatureC != DEVICE_DISCONNECTED_C && sensorValue >= moistureMin && sensorValue <= moistureMax) {
    displayData(temperatureC, sensorValue, val1, val2, val3);
  } else {
    displayError("Error reading data!");
  }

  if (sensorValue < sensorMin || sensorValue > sensorMax) {
    Serial.print("Error: Sensor value (");
    Serial.print(sensorValue);
    Serial.print(") is out of expected range (");
    Serial.print(sensorMin);
    Serial.print(" - ");
    Serial.print(sensorMax);
    Serial.println(")!");
    return;     // Skip the rest of the loop
  }

  int percentage = map(sensorValue, dry, wet, 0, 100);

  Serial.print("Soil Moisture Reading: ");
  Serial.print(sensorValue);
  Serial.print("  Percentage: ");
  Serial.print(percentage);
  Serial.print("%");

  if (percentage <= dryThreshold) {
    Serial.println("  Status: Dry");
  } else {
    Serial.println("  Status: Wet");
  }

  yonDelay(2000);    //  delay(2000);

  loopTime = millis() - loopTime;
  lastSpin = lastSpin + loopTime;
  
  if ((lastSpin) >= autoSpinTime){
    spinIt();
    lastSpin = millis();
    spun = true;
  }

  testTime = millis() - testTime;
  /*Serial.begin(115200);
  delay(2);
  Serial.print("loopTime:");
  Serial.println(loopTime);
  Serial.flush();
  Serial.begin(9600);*/
  if ((millis() - lastInput) >= timeUntilSleep){
      if(spun){
        lastSpin = millis() - lastSpin;
        spun = false;
      }
      esp_sleep_enable_timer_wakeup(time_in_us);
      esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, 0);
      esp_deep_sleep_start();
  }
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
}

byte nitrogen() {
  digitalWrite(DE, HIGH);
  digitalWrite(RE, HIGH);
  yonDelay(10);    //delay(10);
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
  yonDelay(10);     //delay(10);
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
  yonDelay(10);     //delay(10);
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

void spinIt(){
  /* set speed */
  analogWrite(enablePin, motorSpeed);

  /* reference time for delay implementation */
  startTime = millis(); 

  /* Spin one direction */
  digitalWrite(motorPin1, HIGH);
  digitalWrite(motorPin2, LOW);
  refTime = millis() - startTime + clockAdjustment;  
  while(refTime <= spinTime){
    refTime = millis() - startTime + clockAdjustment;
    //Serial.print("refTime: "); Serial.println(refTime);
    //Serial.print("spinTime: "); Serial.println(spinTime);
  }

  startTime = millis(); 
  
  /* Stop spinning */
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, LOW);
  refTime = millis() - startTime + clockAdjustment;
  //Serial.print("refTime: "); Serial.println(refTime);
  while(refTime <= stopTime){
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
  analogWrite(enablePin, 0);

  if (turnButton.pressed){
    lastInput = millis() - lastInput;
  }
  }

  void yonDelay(unsigned int timeDelay){
    startTime = millis();
    refTime = millis() - startTime + clockAdjustment;
    while(refTime <= timeDelay){
      refTime = millis() - startTime + clockAdjustment;
    }
  }
