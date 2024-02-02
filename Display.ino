#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TFT_eSPI.h>
#include <SPI.h>

// Constants for TFT display color
#define TFT_GREY 0x5AEB

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

// Setup function
void setup() {
  Serial.begin(9600);
  tft.init();
  tft.setRotation(2);
  sensors.begin();  // Initialize the Dallas Temperature library
}

// Main loop function
void loop() {
  // Temperature Reading
  sensors.requestTemperatures();
  float temperatureC = sensors.getTempCByIndex(0);

  // Display temperature data or error message
  if (temperatureC != DEVICE_DISCONNECTED_C) {
    displayTemperatureData(temperatureC);
  } else {
    displayError("Error reading temperature!");
  }

  delay(2000);  // Add a delay between temperature and moisture readings

  // Soil Moisture Reading
  int sensorValue = analogRead(soil_moisture_pin);

  // Display soil moisture data or error message
  if (sensorValue < moistureMin || sensorValue > moistureMax) {
    displayError("Error: Sensor value is out of expected range!");
    delay(500); // Add a delay before the next reading
    return;     // Skip the rest of the loop
  }

  int percentage = map(sensorValue, dry, wet, 0, 100);

  // Display soil moisture data and status
  displayMoistureData(sensorValue, percentage, temperatureC);

  delay(2000);  // Adjust the delay based on your application
}

// Display temperature data on TFT
void displayTemperatureData(float temperature) 
{
  tft.fillScreen(TFT_GREY);
  tft.setCursor(10, 10, 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);

  // Display temperature data
  tft.print("Temperature: ");
  tft.print(temperature);
  tft.println(" °C");

    // Display temperature status
  tft.setCursor(10, 30, 2);
  if (temperature <= temperatureCMin) {
    tft.setTextColor(TFT_BLUE, TFT_BLACK);
    tft.print("Temperature Status: Low");
  } else if (temperature >= temperatureCMax) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.print("Temperature Status: High");
  } else {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.print("Temperature Status: Normal");
  }

  tft.println();
}

// Display soil moisture data, percentage, and status on TFT
void displayMoistureData(int sensorValue, int percentage, float temperature) 
{
  tft.fillScreen(TFT_GREY);
  tft.setCursor(10, 10, 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);

  // Display soil moisture data
  tft.print("Soil Moisture Reading: ");
  tft.print(sensorValue);
  tft.println();

 tft.setCursor(10, 30, 2);
  // Display soil moisture percentage
  tft.print("Percentage: ");
  tft.print(percentage);
  tft.println("%");

  tft.setCursor(10, 50, 2);

  // Display soil moisture status
  if (percentage <= dryThreshold) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.print("Status: Dry");
  } else {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.print("Status: Wet");
  }

  tft.println();


}

// Display error message on TFT
void displayError(const char *errorMessage) {
  tft.fillScreen(TFT_GREY);
  tft.setCursor(10, 10, 2);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.setTextSize(1);

  // Display error message
  tft.println(errorMessage);
}
