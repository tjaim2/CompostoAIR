#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TFT_eSPI.h>
#include <SPI.h>

// Constants for TFT display color
#define TFT_BLACK 0x0000
#define TFT_WHITE 0xFFFF

// Pin definitions
#define temperature_sensor_pin 14
#define soil_moisture_pin 12

// OneWire and DallasTemperature setup for temperature sensor
OneWire oneWire(temperature_sensor_pin);
DallasTemperature sensors(&oneWire);

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
const int lowThreshold = 29;

// Function prototypes
void displayError(const char *errorMessage);
void displayData(float temperature, int sensorValue);

// Setup function
void setup() {
  Serial.begin(9600);
  tft.init();
  tft.setRotation(1); // Adjust rotation value here
  sensors.begin();  // Initialize the Dallas Temperature library
}

// Main loop function
void loop() {
  // Temperature Reading
  sensors.requestTemperatures();
  float temperatureC = sensors.getTempCByIndex(0);

  // Soil Moisture Reading
  int sensorValue = analogRead(soil_moisture_pin);

  // Display data or error message
  if (temperatureC != DEVICE_DISCONNECTED_C && sensorValue >= moistureMin && sensorValue <= moistureMax) {
    displayData(temperatureC, sensorValue);
  } else {
    displayError("Error reading data!");
  }

  delay(2000);  // Adjust the delay based on your application
}

// Display temperature and soil moisture data on TFT
void displayData(float temperature, int sensorValue) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);

  // Display header
  tft.setCursor(10, 10, 2);
  tft.setRotation(1); // Adjust rotation value here
  tft.setTextSize(1.5); // Adjust header text size
  tft.print("Compost Air 360 Pro Max");
  tft.setTextSize(1);

  // Display temperature data
  tft.setCursor(10, 40, 2);
  tft.setRotation(1); // Adjust rotation value here
  tft.print("Temperature: ");
  tft.print(temperature);
  tft.println(" °C");

  // Display temperature status
  tft.setCursor(10, 60, 2);
  tft.setRotation(1); // Adjust rotation value here
  tft.print("Status: ");
  if (temperature <= temperatureCMin) {
    tft.setTextColor(TFT_BLUE, TFT_BLACK);
    tft.print("Low");
  } else if (temperature >= temperatureCMax) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.print("High");
  } else {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.print("Normal");
  }

  // Display soil moisture data
  tft.setCursor(160, 40, 2);
  tft.setRotation(1); // Adjust rotation value here
  tft.setTextColor(TFT_WHITE, TFT_BLACK); // Set text color to white for the moisture column
  tft.print("Moisture: ");
  tft.print(sensorValue);
  tft.println();

  // Calculate and display soil moisture percentage
  int percentage = map(sensorValue, dry, wet, 0, 100);
  tft.setCursor(160, 60, 2);
  tft.setRotation(1); // Adjust rotation value here
  tft.print("Percentage: ");
  tft.print(percentage);
  tft.println("%");

  // Display soil moisture status
  tft.setCursor(160, 80, 2);
  tft.setRotation(1); // Adjust rotation value here
  tft.print("Status: ");
  if (percentage <= dryThreshold) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.print("Dry");
  } else {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.print("Wet");
  }

  // Display "Rotate Bin?" text
  tft.setCursor(10, 120, 2);
  tft.setRotation(1); // Adjust rotation value here
  tft.print("Rotate Bin?");

  // Display "YES" text box
  tft.drawRect(100, 120, 40, 20, TFT_WHITE);
  tft.fillRect(100, 120, 40, 20, TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.setCursor(110, 120, 2); // Adjusted position and aligned text
  tft.print("YES");

  // Display "NO" text box  // might comment this out no reason to have NO option
  tft.drawRect(150, 120, 40, 20, TFT_WHITE);
  tft.fillRect(150, 120, 40, 20, TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.setCursor(163, 120, 2); // Adjusted position and aligned text
  tft.print("NO");
}

// Display error message on TFT
void displayError(const char *errorMessage) {
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(10, 10, 2);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.setTextSize(1);

  // Display error message
  tft.println(errorMessage);
}
