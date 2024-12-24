#include <nRF24L01.h>
#include <RF24.h>

// Motor A connections
#define enA 3
#define in1 2
#define in2 4

// Motor B connections
#define enB 6
#define in3 5
#define in4 7

// Ultrasonic sensor pins
#define TRIG_FRONT A2
#define ECHO_FRONT A3
#define TRIG_LEFT A4
#define ECHO_LEFT A5
#define TRIG_RIGHT A0
#define ECHO_RIGHT A1

// nRF24L01 CE and CSN pins
RF24 radio(8, 9);
const byte address[6] = "00001";

// Structure to hold joystick data
struct JoystickData {
  int xAxis;
  int yAxis;
  bool autoMode;
};

JoystickData data;

// Ultrasonic distance thresholds
const int stopRange = 30;      // Distance to stop (in cm)
const int slowDownRange = 80;  // Distance to start slowing down (in cm)

void setup() {
  pinMode(enA, OUTPUT);
  pinMode(enB, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);

  pinMode(TRIG_FRONT, OUTPUT);
  pinMode(ECHO_FRONT, INPUT);
  pinMode(TRIG_LEFT, OUTPUT);
  pinMode(ECHO_LEFT, INPUT);
  pinMode(TRIG_RIGHT, OUTPUT);
  pinMode(ECHO_RIGHT, INPUT);

  Serial.begin(9600);

  if (!radio.begin()) {
    Serial.println("nRF24L01 initialization failed!");
    while (1); // Halt if initialization fails
  }

  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening();
}

void loop() {
  if (radio.available()) {
    radio.read(&data, sizeof(data));

    // Debugging output
    Serial.print("Received X: ");
    Serial.print(data.xAxis);
    Serial.print(" | Y: ");
    Serial.print(data.yAxis);
    Serial.print(" | Mode: ");
    Serial.println(data.autoMode ? "Auto" : "Manual");

    if (data.autoMode) {
      automaticMode();
    } else {
      manualMode(data.xAxis, data.yAxis);
    }
  }
}

void manualMode(int xAxis, int yAxis) {
  if (yAxis < 470) {
    // Backward
    moveBackward(map(yAxis, 470, 0, 0, 255));
  } else if (yAxis > 550) {
    // Forward
    moveForward(map(yAxis, 550, 1023, 0, 255));
  } else if (xAxis < 470) {
    // Turn Right
    moveTurnRight(map(xAxis, 470, 0, 0, 255));
  } else if (xAxis > 550) {
    // Turn Left
    moveTurnLeft(map(xAxis, 550, 1023, 0, 255));
  } else {
    moveStop();
  }
}

void automaticMode() {
  start:
  int frontDistance = getUltrasonicDistance(TRIG_FRONT, ECHO_FRONT);
  int leftDistance = getUltrasonicDistance(TRIG_LEFT, ECHO_LEFT);
  int rightDistance = getUltrasonicDistance(TRIG_RIGHT, ECHO_RIGHT);
  int path =0;
  // Debugging sensor values
  Serial.print("Front: ");
  Serial.print(frontDistance);
  Serial.print(" cm | Left: ");
  Serial.print(leftDistance);
  Serial.print(" cm | Right: ");
  Serial.println(rightDistance);

  if (frontDistance > slowDownRange && leftDistance > stopRange && rightDistance > stopRange) {
    // No obstacles detected, move forward at max speed
    moveForward(150);
    Serial.println("Moving forward at max speed.");
  } else if (frontDistance <= slowDownRange && frontDistance > stopRange) {
    // Gradual slowdown as the obstacle gets closer
    int speed = map(frontDistance, stopRange, slowDownRange, 80, 150);
    moveForward(speed);
    Serial.print("Slowing down, speed: ");
    Serial.println(speed);
  } else if (frontDistance <= stopRange) {
    // Front obstacle detected, stop and decide next move
    moveStop();
    delay(300); // Shorter stop delay
    Serial.println("Obstacle detected in front, stopping.");

    b:

    // Move backward to create space
    moveBackward(120);
    delay(800); // Reduced backward distance
    moveStop();

    // Reevaluate distances after moving backward
    int newLeftDistance = getUltrasonicDistance(TRIG_LEFT, ECHO_LEFT);
    int newRightDistance = getUltrasonicDistance(TRIG_RIGHT, ECHO_RIGHT);
 


    Serial.print("Reevaluating distances. Left: ");
    Serial.print(newLeftDistance);
    Serial.print(" cm | Right: ");
    Serial.println(newRightDistance);

    delay(50); // Shorter stop delay
 
    if (newLeftDistance > newRightDistance && newLeftDistance > stopRange) {
      Serial.println("Turning left to open a new path.");
      moveTurnLeft(120);
      delay(600); // Reduced delay for smaller turn
      moveStop();
    } else if (newRightDistance > stopRange) {
      Serial.println("Turning right to open a new path.");
      moveTurnRight(120);
      delay(600); // Reduced delay for smaller turn
      moveStop();
    } else {
      if(newRightDistance>newLeftDistance)
      {moveTurnRight(100);
      delay(400);
      moveStop();
      }
      else {
      moveTurnLeft(100);
      delay(400);
      moveStop();
      }
      path+=1;
      if (path==3)
{      moveTurnLeft(140);
      delay(900);
      moveStop();
      goto start;
}      
      goto b;
      
      /*
      // If both sides are still blocked, choose a random turn
      Serial.println("Both sides still blocked, turning randomly.");
      if (random(0, 2) == 0) {
        moveTurnLeft(200);
        delay(500); // Reduced delay for smaller turn
      } else {
        moveTurnRight(200);
        delay(500); // Reduced delay for smaller turn
      }
      */
    }
  } 
  else if (leftDistance <= stopRange) {
    // Left obstacle detected, turn right
    Serial.println("Obstacle detected on the left, turning right.");
    moveTurnRight(100);
    delay(300); // Reduced turn time
  }
   else if (rightDistance <= stopRange) {
    // Right obstacle detected, turn left
    Serial.println("Obstacle detected on the right, turning left.");
    moveTurnLeft(100);
    delay(300); // Reduced turn time
  }
   else {
    // Default slow forward motion
    int speed = map(frontDistance, stopRange, slowDownRange, 80, 150);
    moveForward(speed);
    Serial.print("Default slowing down, speed: ");
    Serial.println(speed);
  }
}

void moveForward(int speed) {
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
  analogWrite(enA, speed);
  analogWrite(enB, speed);
}

void moveBackward(int speed) {
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  analogWrite(enA, speed);
  analogWrite(enB, speed);
}

void moveTurnLeft(int speed) {
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
  analogWrite(enA, speed );
  analogWrite(enB, speed);
}

void moveTurnRight(int speed) {
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  analogWrite(enA, speed);
  analogWrite(enB, speed );
}

void moveStop() {
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
  analogWrite(enA, 0);
  analogWrite(enB, 0);
}

int getUltrasonicDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000); // Timeout to prevent long delays
  int distance = duration * 0.034 / 2;          // Convert to cm
  return (distance > 0 && distance <= 300) ? distance : 999;
}
