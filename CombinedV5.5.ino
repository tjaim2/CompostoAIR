#include <Arduino.h>
#include <OneWire.h>
#include "FS.h"
#include <DallasTemperature.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <SoftwareSerial.h>

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Pin definitions
#define temperature_sensor_pin 22
#define soil_moisture_pin 35
#define RE 25
#define DE 33
#define enablePin 27 //13
#define motorPin1 14 //12
#define motorPin2 12 //14
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
void checkTouchButtons();
//void settingsPage();
//void temperaturePage();
//void NPKPage();
//void moisturePage();
//void timerSettingsPage();
void touch_calibrate();


//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// TFT_eSPI setup for TFT display
TFT_eSPI tft = TFT_eSPI();

// Create 15 keys for the keypad
char keyLabel[15][20] = {" ", " ", " ", "SPIN ME", "Settings", "Set Timer", "Temperature Units", "Back", "-", "+", "Timer Units", "Back", " ", " ", " " };
uint16_t keyColor[15] = {TFT_BLACK, TFT_BLACK, TFT_BLACK,
                         TFT_WHITE, TFT_WHITE,  // MAIN MENU COLORS CUT OFF
                         TFT_WHITE, TFT_WHITE, TFT_WHITE,
                         TFT_WHITE, TFT_WHITE, TFT_WHITE,
                         TFT_BLUE, TFT_BLUE, TFT_BLUE,
                         TFT_BLUE
                        };

// Invoke the TFT_eSPI button class and create all the button objects
TFT_eSPI_Button key[15];

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

#define us_from_seconds 1000000  /*micro seconds to seconds */
#define us_from_mins 60000000  /* seconds to minutes*/
#define us_from_hours 3600000000    /*us to hours*/
#define ms_from_seconds 1000  /*micro seconds to seconds */
#define ms_from_mins 60000  /* seconds to minutes*/
#define ms_from_hours 3600000    /*us to hours*/

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


/* Display parameters*/

struct stateDisplay {
  int page;  // used to set what page is being displayed: 0 = Main Menu, 1 = Temp Expanded Screen, 2 = NPK Expanded Screen, 3 = Moisture Expanded Screen, 4 = Settings, 5 = autospin timer settings
  bool pressed;  // flag for touch screen buttons
  bool newPage;  // keeps track of page changes
  bool sensorsUpdated;  // used to avoid screen flickering on refresh
  byte unitsTemp; // set to 0 for celcius , 1 for farenheit
  byte unitsMoisture; // set to 0 for percentage, 1 for absolute values
  byte unitsNPK; // set to 0 for percentage, 1 for absolute values
  unsigned long unitsTime[3];  //Used For Automatic Turning : 0 = seconds, 1 = minutes, 2 = hours
  unsigned long unitsSleep[3]; //Used For Sleep : 0 = seconds, 1 = minutes, 2 = hours
  uint32_t statusColor[4];  // Use for status indicators Green->Yellow->Red
  volatile uint16_t touchX;
  volatile uint16_t touchY;
  char unitChar[2];
};

stateDisplay display = {0, false, true, true, 0, 0, 0, {ms_from_seconds, ms_from_mins, ms_from_hours}, {us_from_seconds, us_from_mins, us_from_hours}, {TFT_GREEN, TFT_YELLOW, TFT_RED, TFT_BLUE}, 0 , 0, {'C', 'F'}};

/* Store Touch Coordinates */
uint16_t touchX;
uint16_t touchY;

/* For rounded Edges */
int32_t cornerRadius = 10;

/* For Menu Printing */
char timerString[4][20] = {"Seconds", "Minutes", "Hours"};

#define MENU_FONT &FreeSansOblique12pt7b // Key label font 1
#define SENSOR_FONT &FreeSansBold12pt7b    // Key label font 2

#define CALIBRATION_FILE "/TouchCalData1"
#define REPEAT_CAL false

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Initialize variables relating to the RTC
RTC_DATA_ATTR uint8_t sleepScalar;     /* ESP32 will go to sleep for this long. Specify units by multiplying by the desired "us_from_XXXX" when defining "time_in_us" below (Ex: if you want units to be seconds, multiply sleepScalar * us_from seconds) */
unsigned long time_in_us;             /* use for esp_deep_sleep function */
unsigned long timeUntilSleep = 30000; /* How long without an input until system goes to sleep (in ms) */
RTC_DATA_ATTR unsigned long autoSpinScalar; /* Scalar Value for Auto Spin Time */
RTC_DATA_ATTR uint8_t autoSpinUnitSelect; /* use with display struct to choose units */
RTC_DATA_ATTR unsigned long autoSpinTime; /* How long between automatic turns (in ms) */
unsigned long lastInput;              /* Tracks when the last user input was (in ms)*/
unsigned long setUpTime;              /* Tracks time setup function (in ms)*/
unsigned long loopTime;               /* Tracks how long each loop is (in ms) */
unsigned long refTime;                /* General use time indexing variable (in ms) */
unsigned long startTime;              /* Time indexing variable (in ms) */
RTC_DATA_ATTR unsigned long lastUpdate;              /* Tracks when the last time sensors were updated */
unsigned long timeSensorRefresh = 10000;
RTC_DATA_ATTR unsigned long testTime;
RTC_DATA_ATTR unsigned long lastSpin = 0;
RTC_DATA_ATTR unsigned long rebootCount = 0;
RTC_DATA_ATTR uint8_t sleepUnitSelect;
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
#define colorBackground TFT_BLACK
#define colorText1 TFT_WHITE


//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

/* struct to store sensor values */
struct stateSensors {
  float temperatureC;
  int moisture;
  byte nitrogen;
  byte phosphorus; 
  byte potassium;
};

stateSensors TMNPK = {0, 0 , 0x00, 0x00, 0x00}; //Temperature Moisture NPK
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void setup() {
  if (rebootCount == 0){
    autoSpinScalar = 20;
    autoSpinUnitSelect = 1;
    sleepUnitSelect = 1;
    sleepScalar = 1;
    lastUpdate = 18446744073709551615;
    time_in_us = sleepScalar * display.unitsSleep[sleepUnitSelect];
    autoSpinTime = autoSpinScalar * display.unitsTime[autoSpinUnitSelect];
  }
  unsigned long rebootTime = sleepScalar * display.unitsSleep[sleepUnitSelect] / 1000;       //sleepScalar assumed to be in seconds

  if (rebootCount != 0){
//    unsigned long rebootTime = sleepScalar * 1000;
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

  touch_calibrate();
/* Update Sensors */
  updateSensors();
  
 /* Set Main Menu */
  setDisplay();
  delay(2000);
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void loop() {
  loopTime = millis();

  // Pressed will be set true is there is a valid touch on the screen
  display.pressed = tft.getTouch(&touchX, &touchY);

  /* Check if display button pressed */
  checkTouchButtons();

   /* Update Sensors */
  updateSensors();

  //Update Display
  setDisplay();

  // Spin the Motor if Button Pushed
  if (turnButton.pressed) {
    spinIt();
    turnButton.pressed = false;
  }
  
  //yonDelay(2000);    //  delay(2000);

  /* Turn Automatically If Necessary */
  loopTime = millis() - loopTime;
  lastSpin = lastSpin + loopTime;
  if ((lastSpin) >= autoSpinTime){
    lastSpin = millis();
    spinIt();
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

  if ((!display.sensorsUpdated) && (!display.newPage)){
    return;
  }

  /* Parameters for left 3 blocks */
  uint16_t leftEdge1 = 58;
  uint16_t topEdge1 = 30;
  uint16_t topEdge2 = 110;
  uint16_t topEdge3 = 190;

   /* Parameters for right 2 blocks */
  uint16_t leftEdge2 = 220;
  uint16_t topEdge4 = 50;
  uint16_t topEdge5 = 170;


/* Improved Version Updating only what needs to be updated (WIP) */
/* Calculate Parameters */
  float nitrogenPercentage = (val1 / maxVal) * 100;
  float phosphorousPercentage = (val2 / maxVal) * 100;
  float potassiumPercentage = (val3 / maxVal) * 100;
  int percentage = map(TMNPK.moisture, dry, wet, 0, 100);
  float tempLow;
  float tempHigh;
  float tempActual;
  if(display.unitsTemp == 0){
    tempLow = temperatureCMin - 5;
    tempHigh = temperatureCMin + 5;
    tempActual = temperature;
  } 
  else if (display.unitsTemp == 1){
    tempLow = (temperatureCMin - 5) * (9 / 5) + 32;
    tempHigh = (temperatureCMin + 5) * (9 / 5) + 32;
    tempActual = temperature * (9 / 5) + 32;
  }

  tft.setTextSize(2);
  
/* Display Temperature Data */
  tft.setCursor(leftEdge1, topEdge1, 1);
  if (tempActual <= tempLow) {  // Adjusted condition for "Low"
    tft.fillRect(3, 3, 204, 76, display.statusColor[3]);
    delay(1);
    tft.setTextColor(colorText1 ,display.statusColor[3]); //tft.setTextColor(display.statusColor[3], TFT_BLACK);
  } else if (tempActual >= tempHigh) {  // Adjusted condition for "High"
    tft.fillRect(3, 3, 204, 76, display.statusColor[2]);
    delay(1);
    tft.setTextColor(colorText1 ,display.statusColor[2]);//tft.setTextColor(display.statusColor[2], TFT_BLACK);
  } else {
    tft.fillRect(3, 3, 204, 76, display.statusColor[0]);
    delay(1);
    tft.setTextColor(colorText1 ,display.statusColor[0]);//tft.setTextColor(display.statusColor[0], TFT_BLACK);
  }
  tft.print(tempActual);
  tft.print(" ");
  tft.println(display.unitChar[display.unitsTemp]);


/* Display NPK Data */
/* Nitrogen */
  tft.setTextSize(3);
  tft.setCursor(34, topEdge2, 1);
  if (nitrogenPercentage >= (2.0 + 0.5)) {
    tft.fillRect(3, 83, 68, 76, display.statusColor[2]);
    delay(1);
    tft.setTextColor(colorText1 ,display.statusColor[2]); //tft.setTextColor(display.statusColor[2], TFT_BLACK);
  } else if (nitrogenPercentage <= (2.0 - 0.5)) {
    tft.fillRect(3, 83, 68, 76, display.statusColor[3]);
    delay(1);
    tft.setTextColor(colorText1 ,display.statusColor[3]); //tft.setTextColor(display.statusColor[1], TFT_BLACK);
  } else {
    tft.fillRect(3, 83, 68, 76, display.statusColor[0]);
    delay(1);
    tft.setTextColor(colorText1 ,display.statusColor[0]); //tft.setTextColor(display.statusColor[0], TFT_BLACK);
  }
  //tft.print(nitrogenPercentage);
  tft.println("N");

/* Phosphorous */
tft.setCursor(103, topEdge2, 1);
if (phosphorousPercentage >= 1.0 + 0.25) {
    tft.fillRect(72, 83, 68, 76, display.statusColor[2]);
    delay(1);
    tft.setTextColor(colorText1 ,display.statusColor[2]); //tft.setTextColor(display.statusColor[2], TFT_BLACK);
} else if (phosphorousPercentage <= 0.5 - 0.25) {
    tft.fillRect(72, 83, 68, 76, display.statusColor[3]);
    delay(1);
    tft.setTextColor(colorText1 ,display.statusColor[3]); //tft.setTextColor(display.statusColor[2], TFT_BLACK);
} else {
    tft.fillRect(72, 83, 68, 76, display.statusColor[0]);
    delay(1);
    tft.setTextColor(colorText1 ,display.statusColor[0]); //tft.setTextColor(display.statusColor[0], TFT_BLACK);
}
  tft.println("P");


/* Potassium */
tft.setCursor(172, topEdge2, 1);
if (potassiumPercentage >= 2.0 + 0.5) {
    tft.fillRect(141, 83, 68, 76, display.statusColor[2]);
    delay(1);
    tft.setTextColor(colorText1 ,display.statusColor[2]); //tft.setTextColor(display.statusColor[2], TFT_BLACK);
} else if (potassiumPercentage <= 2.0 - 0.5) {
    tft.fillRect(141, 83, 68, 76, display.statusColor[3]);
    delay(1);
    tft.setTextColor(colorText1 ,display.statusColor[3]); //tft.setTextColor(display.statusColor[2], TFT_BLACK);
} else {
    tft.fillRect(141, 83, 68, 76, display.statusColor[0]);
    delay(1);
    tft.setTextColor(colorText1 ,display.statusColor[0]); //tft.setTextColor(display.statusColor[0], TFT_BLACK);
}
  tft.println("K");

/* Display Moisture Sensor */
  tft.setCursor(65, topEdge3, 1);
  if (percentage <= (dryThreshold - 5)) {  // Adjusted condition for "Low"
    tft.fillRect(3, 163, 204, 76, display.statusColor[3]);
    delay(1);
    tft.setTextColor(colorText1 ,display.statusColor[3]); //tft.setTextColor(display.statusColor[2], TFT_BLACK);
  } else if (percentage >= (wetThreshold + 5)) {  // Adjusted condition for "High"
    tft.fillRect(3, 163, 204, 76, display.statusColor[2]);
    delay(1);
    tft.setTextColor(colorText1 ,display.statusColor[2]); //tft.setTextColor(display.statusColor[1], TFT_BLACK);
  } else {
    tft.fillRect(3, 163, 204, 76, display.statusColor[0]);
    delay(1);
    tft.setTextColor(colorText1 ,display.statusColor[0]); //tft.setTextColor(display.statusColor[0], TFT_BLACK);
  }
  tft.print(percentage);
  tft.println(" %");
  display.sensorsUpdated = false;
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

void updateSensors(){
  if ((millis() - lastUpdate) > timeSensorRefresh){
    display.sensorsUpdated = true;
    lastUpdate = millis();
    
    // Temperature Reading
    sensors.requestTemperatures();
    TMNPK.temperatureC = sensors.getTempCByIndex(0);

    // Soil Moisture Reading
    TMNPK.moisture = analogRead(soil_moisture_pin);

    // NPK Readings
    TMNPK.nitrogen = 0x64;
    yonDelay(250);    //delay(250);
    TMNPK.phosphorus = 0x64;
    yonDelay(250);    //delay(250);
    TMNPK.potassium = 0x64;
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
    tft.drawCircle(160, 120, 96 , TFT_YELLOW);
    tft.fillCircle(160, 120, 94 , TFT_WHITE);
    tft.fillCircle(160, 120, 92 , TFT_YELLOW);
    tft.fillCircle(160, 120, 90 , TFT_WHITE);
    tft.fillCircle(160, 120, 88 , TFT_YELLOW);
    tft.fillCircle(160, 120, 86 , TFT_SKYBLUE);
    tft.setTextSize(4);
    tft.setCursor(90, 100, 1);
    tft.setTextColor(TFT_YELLOW, TFT_SKYBLUE);
    tft.println("SPLASH");  
  
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void statusMessage(){
    tft.fillScreen(TFT_BLACK);  
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void setDisplay(){

  switch(display.page)
  {
/* Main Menu*/
  case 0: if (display.newPage){
            makeButtons();
            displayData(TMNPK.temperatureC, TMNPK.moisture, TMNPK.nitrogen, TMNPK.phosphorus, TMNPK.potassium);  
            display.newPage = false;
          }
          displayData(TMNPK.temperatureC, TMNPK.moisture, TMNPK.nitrogen, TMNPK.phosphorus, TMNPK.potassium);  
          break;

  /* Temperature Expanded */
  case 1: if (display.newPage){
            display.newPage = false;
            makeButtons();
            }
          break;
          
  /* NPK Expanded */
  case 2: if (display.newPage){
            display.newPage = false;
            makeButtons();
            }
          break;

  /* Moisture Expanded */        
  case 3: if (display.newPage){
            display.newPage = false;
            makeButtons();
            }
          break;

  /* Settings */          
  case 4: if (display.newPage){
            display.newPage = false;
            makeButtons();
            }
          tft.setTextSize(3);
          tft.setCursor(20, 110, 1);
          tft.setTextColor(colorText1, colorBackground);
          tft.println(display.unitChar[display.unitsTemp]);  
          break;

  /* Timer Settings  */          
  case 5: if (display.newPage){
            display.newPage = false;
            makeButtons();
          }
          tft.setTextSize(3);
          tft.setCursor(150, 30, 1);
          tft.setTextColor(colorText1, colorBackground);
          tft.println(autoSpinScalar);  
          tft.setTextSize(2);
          tft.setCursor(110, 100, 1);
          tft.setTextColor(colorText1, colorBackground);
          //tft.fillRect(150, 100, 150, 30, colorBackground);
          tft.println(timerString[sleepUnitSelect]);
          break;

          
  default: 
           break;
  }
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------



//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

/* Makes the button layout and makes them touchable (WIP) */
void makeButtons()
{

switch(display.page)
  {
  /*Main Menu*/
  case 0:   tft.setTextSize(2);
            tft.fillScreen(TFT_BLACK);

            /* TEMPERATURE */
            tft.drawRect(1, 1, 208, 80, colorBorder);
            key[0].initButtonUL(&tft, 1, 1, 208, 80, colorBorder, colorBackground, keyColor[0] , keyLabel[0], 1);
          
          
            /* NPK */
            tft.drawRect(1, 81, 208, 80, colorBorder);
            key[1].initButtonUL(&tft, 1, 81, 208, 80, colorBorder, colorBackground, keyColor[1], keyLabel[1], 1);
          
            /* MOISTURE */
            tft.drawRect(1, 161, 208, 80, colorBorder);
            key[2].initButtonUL(&tft, 1, 161, 208, 80, colorBorder, colorBackground, keyColor[2], keyLabel[2], 1);
          
          
            /* SPIN ME */
            tft.setFreeFont(MENU_FONT);
            tft.drawRect(209, 1, 112, 120, colorBorder);
            key[3].initButtonUL(&tft, 209, 1, 112, 120, colorBorder, colorBackground, keyColor[4], keyLabel[3], 1);
            tft.setCursor(220, 50, 1);
            tft.setTextColor(keyColor[4]);
            tft.println("SPIN ME");
            
            /* SETTINGS */  
            tft.setFreeFont(MENU_FONT);
            tft.drawRect(209, 120, 112, 120, colorBorder);
            key[4].initButtonUL(&tft, 209, 120, 112, 120, colorBorder, colorBackground, keyColor[5], keyLabel[4], 1);
            tft.setTextColor(keyColor[5]);
            tft.setCursor(220, 170, 1);
            tft.println("Settings");
            break;

  
  /* Temperature Screen */
  case 1:           
            break;

  /* NPK Screen */
  case 2:   
            break;

  /* Moisture Screen */
  case 3:   
            break;

  /* Settings Screen */
  case 4:   tft.setTextSize(2);
            tft.fillScreen(colorBackground);
            tft.setFreeFont(MENU_FONT);

            /* Set Automatic Timer */
            tft.drawRect(1, 1, 320, 80, colorBorder);
            key[5].initButtonUL(&tft, 1, 1, 320, 80, colorBorder, colorBackground, keyColor[5] , keyLabel[5], 1);
            tft.setCursor(90, 30, 1);
            tft.setTextColor(colorText1, colorBackground);
            tft.println("Timer Settings");
          
            /* Temperature Units*/
            tft.drawRect(1, 81, 320, 80, colorBorder);
            key[6].initButtonUL(&tft, 1, 81, 320, 80, colorBorder, colorBackground, keyColor[6], keyLabel[6], 1);
            tft.setCursor(140, 110, 1);
            tft.setTextColor(colorText1, colorBackground);
            tft.println("Units");
          
            /* Back Button */
            tft.drawRect(1, 161, 320, 80, colorBorder);
            key[7].initButtonUL(&tft, 1, 161, 320, 80, colorBorder, colorBackground, keyColor[7], keyLabel[7], 1);
            tft.setCursor(140, 190, 1);
            tft.setTextColor(colorText1, colorBackground);
            tft.println("Back");            
            break;
          
/* Timer Settings Screen */
   case 5:  tft.setTextSize(3);//tft.setTextSize(2);
            tft.fillScreen(colorBackground);
            tft.drawRect(1, 1, 320, 80, colorBorder);
            tft.setCursor(150, 30, 1);
            tft.setTextColor(colorText1, colorBackground);
            tft.println(autoSpinScalar);

            /* AUTOSPIN SCALAR - */
            tft.fillRect(1, 1, 64, 80, TFT_LIGHTGREY);
            tft.fillRect(12, 33, 32, 16, TFT_DARKGREY);
            key[8].initButtonUL(&tft, 1, 1, 64, 80, TFT_LIGHTGREY, TFT_LIGHTGREY, keyColor[8] , keyLabel[8], 1);
          
          
            /* AUTOSPIN SCALAR + */
            tft.fillRect(257, 1, 64, 80, TFT_LIGHTGREY);
            tft.fillRect(282, 17, 16, 48, TFT_DARKGREY);
            tft.fillRect(258, 33, 48, 16, TFT_DARKGREY);
            key[9].initButtonUL(&tft, 257, 1, 64, 80, TFT_LIGHTGREY, TFT_LIGHTGREY, keyColor[9], keyLabel[9], 1);
          
            /* Toggle AutoSpin Timer Units */
            tft.setTextSize(2);
            tft.drawRect(1, 81, 320, 80, colorBorder);
            key[10].initButtonUL(&tft, 1, 81, 320, 80, colorBorder, colorBackground, keyColor[10], keyLabel[10], 1);

          
            /* BACK TO MAIN MENU */
            tft.setTextSize(2);
            tft.setFreeFont(MENU_FONT);
            tft.drawRect(1, 161, 320, 80, colorBorder);
            key[11].initButtonUL(&tft, 1, 161, 320, 80, colorBorder, colorBackground, keyColor[11], keyLabel[11], 1);
            tft.setCursor(140, 190, 1);
            tft.setTextColor(colorText1);
            tft.println("Back");
            break;
            
            
    default:  
            break;
  }
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void checkTouchButtons(){

switch(display.page)
  {
/* Main Menu */
  case 0: for (uint8_t b = 0; b < 5; b++) {
            if (display.pressed && key[b].contains(touchX, touchY)) {
              key[b].press(true);  // tell the button it is pressed
            } else {
              key[b].press(false);  // tell the button it is NOT pressed
            }
          }
          
           // Check if any key has changed state
          for (uint8_t b = 0; b < 5; b++) {
        
            if (key[b].justReleased()){      
                if (b == 0) {
                  lastInput = millis();
                  display.page = 1;
                  display.newPage = true;
                }
        
                else if (b == 1) {
                   lastInput = millis();
                   display.page = 2;
                   display.newPage = true;
                }
        
                else if (b == 2){
                  lastInput = millis();
                  display.page = 3;
                  display.newPage = true;
                }
              
                else if (b == 3){
                  lastInput = millis();
                  spinIt();
                }
              
                else if (b == 4){
                  lastInput = millis();
                  display.page = 4;
                  display.newPage = true;
                }
              }
            delay(10); // UI debouncing
            }
            break;

  /* Temperature */
  case 1: 
          break;

  /* NPK Screen */
  case 2: 
            break;
            
   /* Moisture Screen */         
  case 3: 
            break;

  /* Settings Screen */
  case 4: for (uint8_t b = 5; b < 8; b++) {
            if (display.pressed && key[b].contains(touchX, touchY)) {
              key[b].press(true);  // tell the button it is pressed
            } else {
              key[b].press(false);  // tell the button it is NOT pressed
            }
          }
          
          // Check if any key has changed state
          for (uint8_t b = 5; b < 8; b++) {
        
            if (key[b].justReleased()){
              /* Timer Settings */      
                if (b == 5) {
                  lastInput = millis();
                  display.page = 5;
                  display.newPage = true;
               }
               /* Temp Units */
                else if (b == 6) {
                  lastInput = millis();
                  display.unitsTemp = (display.unitsTemp + 1) % 2;
               }
                /* Back to Main Menu */
                else if (b == 7){
                  lastInput = millis();
                  display.page = 0;
                  display.newPage = true;
                }
            }  
              delay(10); // UI debouncing
            }
          break;

  /* TimerSettings Screen */
   case 5: if(display.newPage){
             break;
           }
           for (uint8_t b = 8; b < 12; b++) {
             if (display.pressed && key[b].contains(touchX, touchY)) {
               key[b].press(true);  // tell the button it is pressed
             } else {
               key[b].press(false);  // tell the button it is NOT pressed
             }
           }
          
           // Check if any key has changed state
          for (uint8_t b = 8; b < 12; b++) {
        
            if (key[b].justReleased()){
                    
                if (b == 8) {
                  lastInput = millis();
                  if (autoSpinScalar != 0){
                    autoSpinScalar = autoSpinScalar - 1;
                  }
                }
        
                else if (b == 9) {
                  lastInput = millis();
                  autoSpinScalar = (autoSpinScalar + 1) % 100;
                }
        
                else if (b == 10){
                   lastInput = millis();
                   sleepUnitSelect = (sleepUnitSelect + 1) % 3;
                }
                else if (b == 11){
                  lastInput = millis();
                  display.page = 0;
                  display.newPage = true;
                }
            }
            delay(10); // UI debouncing
            autoSpinTime = autoSpinScalar * display.unitsTime[sleepUnitSelect];
            }
            break;
   default: 
            break;         
  }
}



//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void touch_calibrate()
{
  uint16_t calData[5];
  uint8_t calDataOK = 0;

  // check file system exists
  if (!SPIFFS.begin()) {
    Serial.println("formatting file system");
    SPIFFS.format();
    SPIFFS.begin();
  }

  // check if calibration file exists and size is correct
  if (SPIFFS.exists(CALIBRATION_FILE)) {
    if (REPEAT_CAL)
    {
      // Delete if we want to re-calibrate
      SPIFFS.remove(CALIBRATION_FILE);
    }
    else
    {
      File f = SPIFFS.open(CALIBRATION_FILE, "r");
      if (f) {
        if (f.readBytes((char *)calData, 14) == 14)
          calDataOK = 1;
        f.close();
      }
    }
  }

  if (calDataOK && !REPEAT_CAL) {
    // calibration data valid
    tft.setTouch(calData);
  } else {
    // data not valid so recalibrate
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 0);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    tft.println("Touch corners as indicated");

    tft.setTextFont(1);
    tft.println();

    if (REPEAT_CAL) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.println("Set REPEAT_CAL to false to stop this running again!");
    }

    tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("Calibration complete!");

    // store data
    File f = SPIFFS.open(CALIBRATION_FILE, "w");
    if (f) {
      f.write((const unsigned char *)calData, 14);
      f.close();
    }
  }
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//void settingsPage(){
//  tft.fillScreen(TFT_WHITE);
//  makeButtons();
//}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//
//void temperaturePage(){
//    tft.fillScreen(TFT_RED);
//}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//void NPKPage(){
//    tft.fillScreen(TFT_GREEN);
//}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//
//void moisturePage(){
//    tft.fillScreen(TFT_BLUE);
//}


//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


//void timerSettingsPage(){
//            tft.setTextSize(2);
//            tft.fillScreen(TFT_BLACK);
//            tft.drawRect(1, 1, 320, 80, colorBackground);
//
//
//            /* AUTOSPIN SCALAR - */
//            tft.drawRect(1, 1, 64, 80, TFT_LIGHTGREY);
//            key[8].initButtonUL(&tft, 1, 1, 64, 80, colorBorder, TFT_LIGHTGREY, keyColor[8] , keyLabel[8], 1);
//          
//          
//            /* AUTOSPIN SCALAR + */
//            tft.drawRect(257, 1, 64, 80, TFT_LIGHTGREY);
//            key[9].initButtonUL(&tft, 257, 1, 64, 80, colorBorder, colorBackground, keyColor[9], keyLabel[9], 1);
//          
//            /* AutoSpin Units */
//            tft.drawRect(1, 81, 320, 80, colorBorder);
//            key[10].initButtonUL(&tft, 1, 161, 208, 80, colorBorder, colorBackground, keyColor[10], keyLabel[10], 1);
//            tft.setCursor(150, 190, 1);
//            tft.setTextColor(keyColor[10]);
//            tft.println(timerString[sleepUnitSelect]);
//          
//            /* BACK */
//            tft.setFreeFont(MENU_FONT);
//            tft.drawRect(1, 161, 320, 80, colorBorder);
//            key[11].initButtonUL(&tft, 1, 161, 320, 80, colorBorder, colorBackground, keyColor[11], keyLabel[11], 1);
//            tft.setCursor(220, 190, 1);
//            tft.setTextColor(keyColor[11]);
//            tft.println("BACK");  
//}
