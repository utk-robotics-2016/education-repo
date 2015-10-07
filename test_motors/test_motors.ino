/*
Adafruit Arduino - Lesson 13. DC Motor
*/
#include <Encoder.h>

// motor pins
int motorAdir = 2;
int motorApwm = 11;
int motorBdir = 7;
int motorBpwm = 6;

// encoder variables
Encoder myEnc1(10, 9);
Encoder myEnc2(5,4);

// variables
long oldPosition  = 0;
int speed;

void setup() {
  // pins
  pinMode(motorAdir, OUTPUT);
  pinMode(motorApwm, OUTPUT);
  pinMode(motorBdir, OUTPUT);
  pinMode(motorBpwm, OUTPUT);

  // serial
  Serial.begin(9600);
  Serial.println("Speed 0 to 255");
}

void loop() {
  if (Serial.available()) {
    speed = Serial.parseInt();
    if (speed >= 0 && speed <= 255) {
      analogWrite(motorApwm, speed);
      analogWrite(motorBpwm, speed);
    }
  }
  // printing
  oldPosition = myEnc2.read();
  Serial.println(oldPosition);
}
