#include <Servo.h>

// Motor Pins
#define lmt1 3
#define lmt2 5
#define rmt1 6
#define rmt2 11

// Sensor Pins
#define leftSensor A0
#define rightSensor A1
#define trigPin 8
#define echoPin 9

// Servo Pins
#define servoGripperPin 7
#define servoLifterPin 10

Servo servoGripper;
Servo servoLifter;

float obstacleThreshold = 14.6;
bool hasPlacedObject = false;
bool hasReachedDestination = false; // NEW flag

void setup() {
  pinMode(lmt1, OUTPUT);
  pinMode(lmt2, OUTPUT);
  pinMode(rmt1, OUTPUT);
  pinMode(rmt2, OUTPUT);

  pinMode(leftSensor, INPUT);
  pinMode(rightSensor, INPUT);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  servoGripper.attach(servoGripperPin);
  servoLifter.attach(servoLifterPin);

  servoGripper.write(150);   // closed
  servoLifter.write(0);      // up
}

void loop() {
  // Don't do anything if robot has reached destination
  if (hasReachedDestination) return;

  float distance = averageDistance();

  // Try picking only if object detected and not at destination
  if (distance > 0 && distance <= obstacleThreshold) {
    delay(150);
    float confirmDist = averageDistance();

    if (confirmDist > 0 && confirmDist <= obstacleThreshold) {
      stopMotors();

      bool success = false;
      int maxTries = 4;

      for (int attempt = 1; attempt <= maxTries; attempt++) {
        bool deeper = (attempt >= 2);
        success = tryPick(deeper);
        if (success) break;

        float checkAgain = averageDistance();
        if (checkAgain > obstacleThreshold + 2) break;

        if (attempt < maxTries) {
          adjustPosition();
          delay(300);
        }
      }

      delay(500);
      return;
    }
  }

  // === Line Following ===
  int leftValue = digitalRead(leftSensor);
  int rightValue = digitalRead(rightSensor);

  if (leftValue == 1 && rightValue == 1) {
    moveForward();
  } else if (leftValue == 1 && rightValue == 0) {
    turnRight();
  } else if (leftValue == 0 && rightValue == 1) {
    turnLeft();
  } else {
    stopMotors();
    if (!hasPlacedObject) {
      placeObject();
      hasPlacedObject = true;
      hasReachedDestination = true;  // Set destination flag
    }
  }
}

// === Try to Pick Object ===
bool tryPick(bool deeper) {
  int liftDownLimit = deeper ? 110 : 90;

  // Lower arm and open gripper gradually
  for (int angle = 0; angle <= liftDownLimit; angle++) {
    servoLifter.write(angle);
    if (angle % 3 == 0) {
      int gripperAngle = map(angle, 0, liftDownLimit, 150, 10);
      servoGripper.write(gripperAngle);
    }
    delay(15);
  }
  delay(300);

  // Close gripper to grab
  for (int angle = 10; angle <= 155; angle += 2) {
    servoGripper.write(angle);
    delay(15);
  }
  delay(300);

  // Lift arm
  for (int angle = liftDownLimit; angle >= 0; angle -= 2) {
    servoLifter.write(angle);
    delay(35);
  }
  delay(300);

  float newDist = averageDistance();
  return (newDist > obstacleThreshold + 2); // object removed?
}

// === Drop Function ===
void placeObject() {
  stopMotors();
  delay(300);

  for (int angle = 0; angle <= 90; angle += 2) {
    servoLifter.write(angle);
    delay(30);
  }
  delay(300);

  for (int angle = 150; angle >= 10; angle -= 2) {
    servoGripper.write(angle);
    delay(20);
  }
  delay(300);

  for (int angle = 90; angle >= 0; angle -= 2) {
    servoLifter.write(angle);
    delay(30);
  }
  delay(300);

  servoGripper.write(150); // close again
  delay(200);
}

// === Adjust Position Based on Distance ===
void adjustPosition() {
  stopMotors();
  delay(200);

  float d = averageDistance();

  if (d > 0 && d <= obstacleThreshold) {
    // Too close — reverse
    analogWrite(lmt1, 255); analogWrite(lmt2, 0);
    analogWrite(rmt1, 0);   analogWrite(rmt2, 255);
    delay(350);
  } else if (d > obstacleThreshold && d <= obstacleThreshold + 5) {
    // Slightly far — move forward
    analogWrite(lmt1, 0);   analogWrite(lmt2, 255);
    analogWrite(rmt1, 255); analogWrite(rmt2, 0);
    delay(350);
  }

  stopMotors();
  delay(300);
}

// === Distance Helpers ===
int getDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);
  if (duration == 0) return -1;
  return duration * 0.034 / 2;
}

float averageDistance() {
  int d1 = getDistance(); delay(10);
  int d2 = getDistance(); delay(10);
  int d3 = getDistance();

  if (d1 < 0 || d2 < 0 || d3 < 0) return -1;
  return (d1 + d2 + d3) / 3.0;
}

// === Movement ===
void moveForward() {
  analogWrite(lmt1, 0);   analogWrite(lmt2, 240);
  analogWrite(rmt1, 240); analogWrite(rmt2, 0);
}

void turnLeft() {
  analogWrite(lmt1, 240); analogWrite(lmt2, 0);
  analogWrite(rmt1, 240); analogWrite(rmt2, 0);
}

void turnRight() {
  analogWrite(lmt1, 0);   analogWrite(lmt2, 240);
  analogWrite(rmt1, 0);   analogWrite(rmt2, 240);
}

void stopMotors() {
  analogWrite(lmt1, 0); analogWrite(lmt2, 0);
  analogWrite(rmt1, 0); analogWrite(rmt2, 0);
}
