// Motor Connections (ENA must use PWM pin)
#define IN1 4
#define IN2 5
#define ENA 15

void motorAccel() {
  for (int i = 0; i < 256; i++) {
    analogWrite(ENA, i);
    delay(20);
  }
}

void motorDecel() {
  for (int i = 255; i >= 0; --i) {
    analogWrite(ENA, i);
    delay(20);
  }
}

void setMotorSpeed(int speed) {
  analogWrite(ENA, speed);
}

void moveMotorSlowly() {
  // Gradually accelerate
  for (int speed = 0; speed <= 60; speed++) {
    setMotorSpeed(speed * 2.55);  // Convert to 0-255 range
    delay(50);
  }

  delay(500);  // Wait at full speed for a moment

  // Gradually decelerate
  for (int speed = 60; speed >= 0; speed--) {
    setMotorSpeed(speed * 2.55);  // Convert to 0-255 range
    delay(50);
  }
}

void setup() {
  // Set motor connections as outputs
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);

  // Start with motors off
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  setMotorSpeed(0);
}

void loop() {
  // Set motors forward
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  // Move motor slowly
  moveMotorSlowly();

  delay(10);  // Wait before reversing

  // Set motors reverse
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  // Move motor slowly in reverse
  moveMotorSlowly();

  delay(10);  // Wait before stopping
}
