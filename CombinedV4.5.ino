#include <Arduino.h>
#include <OneWire.h>
#include "FS.h"
#include <DallasTemperature.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <SoftwareSerial.h>

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Pin definitions
#define temperature_sensor_pin 14
#define soil_moisture_pin 12
#define RE 25
#define DE 26
#define enablePin 27 //13
#define motorPin1 33 //12
#define motorPin2 14 //14
#define spinButton 13 //27
#define resetPin 4

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// OneWire and DallasTemperature setup for temperature sensor
OneWire oneWire(temperature_sensor_pin);
DallasTemperature sensors(&oneWire);

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Function prototypes
void displayError(const char *errorMessage);
void displayData(float temperature, int sensorValue, byte val1, byte val2, byte val3);
void spinIt();
void yonDelay();
void splashScreen();
void statusMessage();
void makeButtons();
void setDisplay();
void updateSensors();
//void settingsPage();
//void temperaturePage();
//void NPKPage();
//void moisturePage();

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// TFT_eSPI setup for TFT display
TFT_eSPI tft = TFT_eSPI();

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

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

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Modbus RTU requests for reading NPK values
const byte nitro[] = {0x01, 0x03, 0x00, 0x1e, 0x00, 0x01, 0xe4, 0x0c};
const byte phos[] = {0x01, 0x03, 0x00, 0x1f, 0x00, 0x01, 0xb5, 0xcc};
const byte pota[] = {0x01, 0x03, 0x00, 0x20, 0x00, 0x01, 0x85, 0xc0};

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

byte values[11];
SoftwareSerial mod(26, 32);

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Define variables relating to the RTC
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
unsigned long lastUpdate = 100000;              /* Tracks when the last time sensors were updated */
RTC_DATA_ATTR unsigned long testTime;
RTC_DATA_ATTR unsigned long lastSpin = 0;
RTC_DATA_ATTR unsigned long rebootCount = 0;
bool spun = false;

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

/* Variables That Adjust motor timing and speed */
const int motorSpeed = 255;   /* [0 - 255] [off - fullspeed] */
const unsigned long spinTime = 2000;    /*  time Motor is spinning (in milliseconds) */
const unsigned long stopTime = 1000;    /* Time off between consecutive spins in same spin cycle (in milliseconds) */
const unsigned long clockAdjustment = 1; /* Ignore this: Used in SpinIt() function, minor adjustment to motor timing to compensate for instruction cycle limitations */

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

/* General struct for button */
struct Button {
  const int PIN;
  unsigned long timeLastPressed;
  bool pressed;
  unsigned long debounceDelay; // in (ms)
};

/* Spin button */
Button turnButton = {spinButton, 0, false, 75};

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void IRAM_ATTR isr() {
    unsigned long timePressed = millis();
    if (((timePressed - turnButton.timeLastPressed) > turnButton.debounceDelay) && (turnButton.pressed == false)) {
      turnButton.pressed = true;
      turnButton.timeLastPressed = timePressed;
      lastInput = millis();
      lastSpin = lastInput;
      spun = true;
    }
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

/* Struct for display parameters*/
struct stateDisplay {
  int page;  // used to set what page is being displayed: 0 = Main Menu, 1 = Settings, 2 = Temp Expanded Screen, 3 = NPK Expanded Screen, 4 = Moisture Expanded Screen
  bool newPage;  // keeps track of page changes
  int unitsTemp; // set to 0 for celcius , 1 for farenheit
  int unitsMoisture; // set to 0 for percentage, 1 for absolute values
  int unitsNPK; // set to 0 for percentage, 1 for absolute values
  uint32_t statusColor[3];  // Use for status indicators Green->Yellow->Red
};

stateDisplay display = {0, false, 0, 0, 0, {TFT_GREEN, TFT_YELLOW, TFT_RED}};

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Constants for TFT display color
#define TFT_BLACK 0x0000
#define TFT_WHITE 0xFFFF
#define colorTemp TFT_BLACK //TFT_RED
#define colorNPK TFT_BLACK //TFT_GREEN
#define colorMoist  TFT_BLACK//TFT_BLUE
#define colorSpin  TFT_BLACK
#define colorSettings  TFT_BLACK
#define colorBorder TFT_WHITE


//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

/* struct to store sensor values */
struct stateSensors {
  float temperatureC;
  int moisture;
  byte nitrogen;
  byte phosphorous; 
  byte potassium;
};

stateSensors TMNPK = {0, 0 , 0x00, 0x00, 0x00}; //Temperature Moisture NPK
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void setup() {
  unsigned long rebootTime = sleep_time * us_from_seconds / 1000;       //sleep_time assumed to be in seconds
  if (rebootCount != 0){
//    unsigned long rebootTime = sleep_time * 1000;
    lastSpin = lastSpin + rebootTime;      
  }
  rebootCount++;
  lastInput = millis();

  /* Initiate Display */
  pinMode(resetPin, OUTPUT);
  digitalWrite(resetPin, HIGH);
  Serial.begin(9600);
  tft.init();
  tft.setRotation(1);
  sensors.begin();
  mod.begin(9600);
  splashScreen();

  /* Initialize Sensors */
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
  
  }

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void loop() {
  loopTime = millis();
 
 /* Update Sensors */
  updateSensors();

  //Update Display
  setDisplay();

    // Spin the Motor if Button Pushed
  if (turnButton.pressed) {
    spinIt();
    turnButton.pressed = false;
  }

  int percentage = map(TMNPK.moisture, dry, wet, 0, 100);

  yonDelay(2000);    //  delay(2000);

  /* Turn Automatically If Necessary */
  loopTime = millis() - loopTime;
  lastSpin = lastSpin + loopTime;
  if ((lastSpin) >= autoSpinTime){
    spinIt();
    lastSpin = millis();
    spun = true;
  }
  
/* Go to Sleep*/
  if ((millis() - lastInput) >= timeUntilSleep){
      digitalWrite(resetPin, LOW);
      if(spun){
        lastSpin = millis() - lastSpin;
        spun = false;
      }
      esp_sleep_enable_timer_wakeup(time_in_us);
      esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, 0);
      esp_deep_sleep_start();
  }
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void displayData(float temperature, int sensorValue, byte val1, byte val2, byte val3) {

  /* Parameters for left 3 blocks */
   uint16_t leftEdge1 = 58;
   uint16_t topEdge1 = 30;
   uint16_t topEdge2 = 110;
   uint16_t topEdge3 = 190;

   /* Parameters for right 2 blocks */
   uint16_t leftEdge2 = 220;
   uint16_t topEdge4 = 50;
   uint16_t topEdge5 = 170;

   /* Draw The Menu */
   tft.setTextSize(2);
   tft.fillScreen(TFT_BLACK);
   tft.drawRect(1, 1, 208, 80, colorBorder);
   tft.drawRect(1, 81, 208, 80, colorBorder);
   tft.drawRect(1, 161, 208, 80, colorBorder);
   tft.drawRect(209, 1, 112, 120, colorBorder);
   tft.drawRect(209, 120, 112, 120, colorBorder);
   tft.setCursor(leftEdge1, topEdge1, 1);
   tft.print(temperature);
   tft.println("%");
   tft.setCursor(leftEdge1, topEdge2);
   tft.print(temperature);
   tft.println("%");
   tft.setCursor(leftEdge1, topEdge3);
   tft.print(temperature);
   tft.println("%");
   tft.setCursor(leftEdge2, topEdge4);
   tft.println("SPIN ME");
   tft.setCursor(leftEdge2, topEdge5);
   tft.println("Settings");

  /* Improved Version Updating only what needs to be updated (WIP) */
  /* Update Data on Display */  
  // tft.setTextSize(2);
  // Serial.println("TexTsize done ");
  // tft.setCursor(1, 30, 1);
  // tft.print(temperature);
  // tft.println("%");
  // Serial.println("1s senseir done ");
  // tft.setCursor(1, 220);
  // tft.print(temperature);
  // tft.println("%");
  // tft.setCursor(1, 190);
  // tft.print(temperature);
  // tft.println("%");
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void displayError(const char *errorMessage) {
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(10, 10, 2);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.setTextSize(1);
  tft.println(errorMessage);
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void setDisplay(){

  switch(display.page)
  {
  case 0: if (display.newPage){
            //  makeButtons();
            display.newPage = false;
            }
            displayData(TMNPK.temperatureC, TMNPK.moisture, TMNPK.nitrogen, TMNPK.phosphorous, TMNPK.potassium);  
            break;
  case 1: if (display.newPage){
            display.newPage = false;
            }
//          settingsPage();
          break;
  case 2: if (display.newPage){
            display.newPage = false;
            }
//          temperaturePage();
          break;
  case 3: if (display.newPage){
            display.newPage = false;
            }
//          NPKPage();
          break;
  case 4: if (display.newPage){
            display.newPage = false;
            }
//          moisturePage();
          break;
  default: if (display.newPage){
            //  makeButtons();
            display.newPage = false;
            }
            displayData(TMNPK.temperatureC, TMNPK.moisture, TMNPK.nitrogen, TMNPK.phosphorous, TMNPK.potassium);  
            break;
  }
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void updateSensors(){
  if ((millis() - lastUpdate) > 2000){
    lastUpdate = millis();
    // Temperature Reading
    sensors.requestTemperatures();
    TMNPK.temperatureC = sensors.getTempCByIndex(0);

    // Soil Moisture Reading
    TMNPK.moisture = analogRead(soil_moisture_pin);

    // NPK Readings
    TMNPK.nitrogen = nitrogen();
    yonDelay(250);    //delay(250);
    TMNPK.phosphorous = phosphorous();
    yonDelay(250);    //delay(250);
    TMNPK.potassium = potassium();
    yonDelay(250);    //delay(250);
  }
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

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

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

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

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

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

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

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

  //--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

  void yonDelay(unsigned int timeDelay){
    startTime = millis();
    refTime = millis() - startTime + clockAdjustment;
    while(refTime <= timeDelay){
      refTime = millis() - startTime + clockAdjustment;
    }
  }

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void splashScreen() {
    tft.fillScreen(TFT_SKYBLUE);
//    tft.drawCircle(int32_t x, int32_t y, int32_t r, uint32_t color);

    
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void statusMessage(){
    tft.fillScreen(TFT_BLACK);  
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//void settingsPage();

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//void temperaturePage();

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//void NPKPage();

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//void moisturePage();

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

/* Makes the button layout and makes them touchable (WIP) */
// void makeButtons()
// {

//   /* Draw The Buttons*/
//   tft.setTextSize(2);
//   tft.fillScreen(TFT_BLACK);

//   /* B1 */
//   tft.drawRect(1, 1, 208, 80, colorBorder);
//   key[1].initButton(&tft,1, 1, 208, 80, TFT_WHITE, TFT_WHITE, TFT_WHITE, " o baby", 1);


//   /* B2*/
//   tft.drawRect(1, 81, 208, 80, colorBorder);
//   key[2].initButton(&tft,1, 81, 208, 80, colorBorder, TFT_BLACK, TFT_WHITE, " o baby ", 1);

//   /* B3*/
//   tft.drawRect(1, 161, 208, 80, colorBorder);
//   key[3].initButton(&tft,1, 161, 208, 80, colorBorder, TFT_BLACK, TFT_WHITE, " o baby ", 1);


//   /* B4*/
//   tft.drawRect(209, 1, 112, 120, colorBorder);
//   key[4].initButton(&tft,209, 1, 112, 120, colorBorder, TFT_BLACK, TFT_WHITE, " o", 1);
//   tft.setCursor(220, 50);
//   tft.println("SPIN ME");
  
//   /* B5*/  
//   tft.drawRect(209, 120, 112, 120, colorBorder);
//   key[5].initButton(&tft,209, 120, 112, 120, colorBorder, TFT_BLACK, TFT_WHITE, "o ", 1);
//   tft.setCursor(220, 170);
//   tft.println("Settings");

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
