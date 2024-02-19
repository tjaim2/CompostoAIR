#include <SoftwareSerial.h>
#include <dummy.h>
#include <Wire.h>
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>


// Pins Go Here
// Motor Pins
#define enablePin 27
#define motorPin1 21
#define motorPin2 22
#define spinButton 13
// Temperature and Moisture Sensor
#define temperature_sensor_pin 4
#define soil_moisture_pin 15

#define sleep_time  5        /* Time ESP32 will go to sleep for this long. Specify units by selecting from the "us_from_XXXX" parameters below when defining "time_in_us" */

/* Included Various Conversion Factors for easier adjustments */
#define us_from_seconds 1000000  /*micro seconds to seconds */
#define us_from_mins 60000000  /* seconds to minutes*/
#define us_from_hours 3600000000    /*us to hours*/

int time_in_us = sleep_time * us_from_seconds;

/* Use these parameters to adjust the motor timing and speed */
int motorSpeed = 255;   /* [0 - 255] [off - fullspeed] */
int spinTime = 2000;    /*  time Motor is spinning (in milliseconds) */
int timeBetweenSpins = 1000;    /* Off Time between switch in direction (in milliseconds) */

//RTC_DATA_ATTR int bootCount = 0;

// NPK Sensor
// Modbus RTU requests for reading NPK values
const byte nitro[] = {0x01,0x03, 0x00, 0x1e, 0x00, 0x01, 0xe4, 0x0c};
const byte phos[] = {0x01,0x03, 0x00, 0x1f, 0x00, 0x01, 0xb5, 0xcc};
const byte pota[] = {0x01, 0x03, 0x00, 0x20, 0x00, 0x01, 0x85, 0xc0};

// A variable used to store NPK values
byte values[11];

// Sets up a new SoftwareSerial object modbus
// Digital pins 10 and 11 should be used with a Mega or Mega 2560
SoftwareSerial mod(32, 33);

// Temperature Sensor 
OneWire oneWire(temperature_sensor_pin);
DallasTemperature sensors(&oneWire);

const int dry = 2830; // value for dry sensor
const int wet = 1030; // value for wet sensor
const int sensorMin = 300; // minimum expected sensor value
const int sensorMax = 3000; // maximum expected sensor value
const int dryThreshold = 40;


void setup() {
  // Set the baud rate for the Serial port
  Serial.begin(9600);

  sensors.begin();  // Initialize the Dallas Temperature library

  // Set the baud rate for the SerialSoftware object
  mod.begin(9600);

  // Define pin modes for RE and DE
  pinMode(RE, OUTPUT);
  pinMode(DE, OUTPUT);
  
  delay(200);
  Serial.flush();

  
  Serial.begin(115200);
  delay(500);

  /* Configure Pins for motor control */
  pinMode(motorPin1, OUTPUT);     /* MotorPins Adjust Direction */
  pinMode(motorPin2, OUTPUT);
  pinMode(enablePin, OUTPUT);     /* Speed Adjustment */
  pinMode(spinButton, INPUT_PULLUP);
  
  esp_sleep_enable_timer_wakeup(time_in_us);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, 0);
}

void spinIt(){
  /* set speed using values [0 - 255] */
  analogWrite(enablePin, motorSpeed);
  
  /* Spin one direction */
  digitalWrite(motorPin1, HIGH);
  digitalWrite(motorPin2, LOW);
  delay(spinTime);
  
  /* Stop spinning */
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, LOW);
  delay(timeBetweenSpins);
  
  /* Spin opposite direction */
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, HIGH);
  delay(spinTime);
  
  /* Stop spinning */
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, LOW);
  delay(timeBetweenSpins);
}

void loop() {
  // Read values
  byte val1,val2,val3;
  val1 = nitrogen();
  delay(250);
  val2 = phosphorous();
  delay(250);
  val3 = potassium();
  delay(250);

  float maxVal = 1999;

  Serial.print("Nitrogen: ");
  Serial.print((val1/maxVal)*100);
  Serial.println(" %");
  Serial.print("Phosphorous: ");
  Serial.print((val2/maxVal)*100);
  Serial.println(" %");
  Serial.print("Potassium: ");
  Serial.print((val3/maxVal)*100);
  Serial.println(" %");
  
  delay(3000);

  // Temperature Reading
  sensors.requestTemperatures();
  float temperatureC = sensors.getTempCByIndex(0);

  if (temperatureC != DEVICE_DISCONNECTED_C) {
    Serial.print("Temperature: ");
    Serial.print(temperatureC);
    Serial.println(" °C");
  } else {
    Serial.println("Error reading temperature!");
  }

  // Soil Moisture Reading
  int sensorValue  = analogRead(soil_moisture_pin);

  if (sensorValue < sensorMin || sensorValue > sensorMax) {
    Serial.print("Error: Sensor value (");
    Serial.print(sensorValue);
    Serial.print(") is out of expected range (");
    Serial.print(sensorMin);
    Serial.print(" - ");
    Serial.print(sensorMax);
    Serial.println(")!");
    delay(500); // Add a delay before the next reading
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

  delay(500);

  spinIt();
  esp_deep_sleep_start();
  }

  byte nitrogen(){
  digitalWrite(DE,HIGH);
  digitalWrite(RE,HIGH);
  delay(10);
  if(mod.write(nitro,sizeof(nitro))==8){
    digitalWrite(DE,LOW);
    digitalWrite(RE,LOW);
    for(byte i=0;i<7;i++){
    //Serial.print(mod.read(),HEX);
    values[i] = mod.read();
    //Serial.print(values[i],HEX);
    }
    Serial.println();
  }
  return values[4];
}
 
byte phosphorous(){
  digitalWrite(DE,HIGH);
  digitalWrite(RE,HIGH);
  delay(10);
  if(mod.write(phos,sizeof(phos))==8){
    digitalWrite(DE,LOW);
    digitalWrite(RE,LOW);
    for(byte i=0;i<7;i++){
    //Serial.print(mod.read(),HEX);
    values[i] = mod.read();
    //Serial.print(values[i],HEX);
    }
    Serial.println();
  }
  return values[4];
}
 
byte potassium(){
  digitalWrite(DE,HIGH);
  digitalWrite(RE,HIGH);
  delay(10);
  if(mod.write(pota,sizeof(pota))==8){
    digitalWrite(DE,LOW);
    digitalWrite(RE,LOW);
    for(byte i=0;i<7;i++){
    //Serial.print(mod.read(),HEX);
    values[i] = mod.read();
    //Serial.print(values[i],HEX);
    }
    Serial.println();
  }
  return values[4];
}
