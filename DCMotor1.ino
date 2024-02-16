#define enablePin 27
#define motorPin1 21
#define motorPin2 22
#define spinButton 13

#define sleep_time  5        /* Time ESP32 will go to sleep for this long. Time units are specified by selecting from the parameters below when defining time_in_us */

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

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // How many sleep cycles
  //++bootCount;
  //Serial.println("Boot number: " + String(bootCount));

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
  spinIt();
  esp_deep_sleep_start();
  }
  
