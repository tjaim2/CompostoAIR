#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define temperature_sensor_pin 4
#define soil_moisture_pin 15

OneWire oneWire(temperature_sensor_pin);
DallasTemperature sensors(&oneWire);

const int dry = 2830; // value for dry sensor
const int wet = 1030; // value for wet sensor
const int sensorMin = 300; // minimum expected sensor value
const int sensorMax = 3000; // maximum expected sensor value
const int dryThreshold = 40;

void setup() {
  Serial.begin(9600);

  sensors.begin();  // Initialize the Dallas Temperature library
}

void loop() {
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

  delay(800);
}
