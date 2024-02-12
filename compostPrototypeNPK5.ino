#include <SoftwareSerial.h>

#include <dummy.h>




#include <Wire.h>
#include <Arduino.h>
//#include <OneWire.h>

// RE and DE Pins set the RS485 module
// to Receiver or Transmitter mode
#define RE 25
#define DE 26

// Modbus RTU requests for reading NPK values
//const byte nitro[] = {0x01, 0x03, 0x00, 0x1e, 0x00, 0x01, 0xb5, 0xcc};
//const byte phos[] = {0x01, 0x03, 0x00, 0x1f, 0x00, 0x01, 0xe4, 0x0c};
const byte nitro[] = {0x01,0x03, 0x00, 0x1e, 0x00, 0x01, 0xe4, 0x0c};
const byte phos[] = {0x01,0x03, 0x00, 0x1f, 0x00, 0x01, 0xb5, 0xcc};
const byte pota[] = {0x01, 0x03, 0x00, 0x20, 0x00, 0x01, 0x85, 0xc0};

// A variable used to store NPK values
byte values[11];

// Sets up a new SoftwareSerial object modbus
// Digital pins 10 and 11 should be used with a Mega or Mega 2560
//SoftwareSerial mod(2, 3);
SoftwareSerial mod(32, 33);
 
void setup() {
  // Set the baud rate for the Serial port
  Serial.begin(9600);

  // Set the baud rate for the SerialSoftware object
  mod.begin(9600);

  // Define pin modes for RE and DE
  pinMode(RE, OUTPUT);
  pinMode(DE, OUTPUT);
  
  delay(500);
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

  // Print values to the serial monitor
/*  Serial.print("Nitrogen: ");
  Serial.print(val1);
  Serial.println(" mg/kg");
  Serial.print("Phosphorous: ");
  Serial.print(val2);
  Serial.println(" mg/kg");
  Serial.print("Potassium: ");
  Serial.print(val3);
  Serial.println(" mg/kg");
*/

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
