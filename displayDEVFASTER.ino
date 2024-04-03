#include <TFT_eSPI.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include "FS.h"
#include <stdio.h>


//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

const int dry = 2830;
const int wet = 1030;
unsigned long refTime;                /* General use time indexing variable (in ms) */
unsigned long startTime;              /* Time indexing variable (in ms) */
const unsigned long clockAdjustment = 1; /* Ignore this: Used in SpinIt() function, minor adjustment to motor timing to compensate for instruction cycle limitations */

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

/* Used for Display Main Page */
#define colorTemp TFT_BLACK //TFT_RED
#define colorNPK TFT_BLACK //TFT_GREEN
#define colorMoist  TFT_BLACK//TFT_BLUE
#define colorSpin  TFT_BLACK
#define colorSettings  TFT_BLACK
#define colorBorder TFT_WHITE
#define resetPin 4

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

/* Struct for display parameters*/
struct systemState {
  const int page;  // used to set what page is being displayed: 1 = Main Menu, 2 = Settings, 3 = Temp Expanded Screen, 4 = NPK Expanded Screen, 5 = Moisture Expanded Screen
  int unitsTemp; // set to 1 for celcius , 2 for farenheit
  int unitsMoisture; // set to 1 for percentage, 2 for absolute values
  int unitsNPK; // set to 1 for percentage, 2 for absolute values
  uint32_t statusColor[3];
};

systemState display = {1, 1, 1, 1, {TFT_GREEN, TFT_YELLOW, TFT_RED}};
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// TFT_eSPI setup for TFT display
TFT_eSPI tft = TFT_eSPI();



// Create 15 keys for the keypad
char keyLabel[15][5] = {"New", "Del", "Send", "1", "2", "3", "4", "5", "6", "7", "8", "9", ".", "0", "#" };
uint16_t keyColor[15] = {TFT_RED, TFT_DARKGREY, TFT_DARKGREEN,
                         TFT_BLUE, TFT_BLUE, TFT_BLUE,
                         TFT_BLUE, TFT_BLUE, TFT_BLUE,
                         TFT_BLUE, TFT_BLUE, TFT_BLUE,
                         TFT_BLUE, TFT_BLUE, TFT_BLUE
                        };

// Invoke the TFT_eSPI button class and create all the button objects
//TFT_eSPI_Button key[15];

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

/* Function Prototypes */
void displayError(const char *errorMessage);
void yonDelay();
void splashScreen();
void statusMessage();
void displayData(float temperature, int sensorValue, byte val1, byte val2, byte val3);
void makeButtons();

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void setup() {
  pinMode(resetPin, OUTPUT);
  digitalWrite(resetPin, HIGH);
  Serial.begin(9600);
  tft.init();
  tft.setRotation(1);
  //splashScreen();
  //makeButtons();
  Serial.println("setup done ");
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void loop() {
//sensors.requestTemperatures();
  float temperatureC = 100.00; //sensors.getTempCByIndex(0);

  // Soil Moisture Reading
  int sensorValue = 100; //analogRead(soil_moisture_pin);

  // NPK Readings
  byte val1, val2, val3;
  val1 = 0;
  yonDelay(250);    //delay(250);
  val2 = 0;
  yonDelay(250);    //delay(250); 
  val3 = 0;
  yonDelay(250);    //delay(250);
  Serial.println("Variables done ");
 
  displayData(temperatureC, sensorValue, val1, val2, val3);
  int percentage = map(sensorValue, dry, wet, 0, 100);
  Serial.println("Loop done \n");
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


}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void statusMessage(){
    tft.fillScreen(TFT_BLACK);  
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void displayData(float temperature, int sensorValue, byte val1, byte val2, byte val3) {
/*
  tft.setTextSize(2);
  Serial.println("TexTsize done ");
  tft.setCursor(1, 30, 1);
  tft.print(temperature);
  tft.println("%");
  Serial.println("1s senseir done ");
  tft.setCursor(1, 220);
  tft.print(temperature);
  tft.println("%");
  tft.setCursor(1, 190);
  tft.print(temperature);
  tft.println("%");
*/
 tft.setTextSize(2);
  uint16_t leftEdge1 = 58;//104 - tft.textWidth(block1);
  uint16_t topEdge1 = 30;// - tft.fontHeight();
  uint16_t topEdge2 = 110;
  uint16_t topEdge3 = 190;
   
  uint16_t leftEdge2 = 220;
  uint16_t topEdge4 = 50;
  uint16_t topEdge5 = 170;
  //uint16_t leftEdge1 = 104 - ceil(tft.textWidth(block1) / 2);
  //uint16_t topEdge1 = 40 - ceil(tft.fontHeight());
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
  return;
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/* PROTOTYPING ONLY */
// void makeButtons()
// {

//   /* Draw The Buttons*/
//   tft.setTextSize(2);
//   tft.fillScreen(TFT_BLACK);

//   /* B1 */
//   tft.drawRect(1, 1, 208, 80, colorBorder);


//   /* B2*/
//   tft.drawRect(1, 81, 208, 80, colorBorder);

//   /* B3*/
//   tft.drawRect(1, 161, 208, 80, colorBorder);


//   /* B4*/
//   tft.drawRect(209, 1, 112, 120, colorBorder);
//   tft.setCursor(220, 50);
//   tft.println("SPIN ME");
  
//   /* B5*/  
//   tft.drawRect(209, 120, 112, 120, colorBorder);
//   tft.setCursor(220, 170);
//   tft.println("Settings");
//   return;
//}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/* REAL FUNCTION (WIP) */

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


// }

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------



//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
