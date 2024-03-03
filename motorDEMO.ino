#include <Arduino.h>

// Pin definitions
#define enablePin 27
#define motorPin1 21
#define motorPin2 22
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

void setup() {
  Serial.begin(115200);
  
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
