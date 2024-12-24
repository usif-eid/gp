#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// nRF24L01 CE and CSN pins
RF24 radio(8, 9);
const byte address[6] = "00001"; // Communication address

// Joystick pins
#define JOY_X A0
#define JOY_Y A1
#define JOY_SW 2 // Joystick button for toggling modes

// Structure to hold joystick data
struct JoystickData {
  int xAxis;
  int yAxis;
  bool autoMode; // Mode flag: true = auto, false = manual
};

JoystickData data = {512, 512, false}; // Initialize structure with default values

void setup() {
  pinMode(JOY_SW, INPUT_PULLUP);

  Serial.begin(9600);
  if (!radio.begin()) {
    Serial.println("nRF24L01 initialization failed!");
    while (1); // Halt execution if initialization fails
  }
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_LOW);
  radio.stopListening(); // Set to transmitter mode
}

void loop() {
  // Read joystick axes
  data.xAxis = analogRead(JOY_X); // X-axis
  data.yAxis = analogRead(JOY_Y); // Y-axis

  // Read joystick button (toggle auto/manual mode)
  if (digitalRead(JOY_SW) == LOW) {
    delay(300); // Debounce delay
    data.autoMode = !data.autoMode; // Toggle mode
  }

  // Send joystick data
  bool success = radio.write(&data, sizeof(data));

  // Debugging output
  if (success) {
    Serial.print("Sent X: ");
    Serial.print(data.xAxis);
    Serial.print(" | Y: ");
    Serial.print(data.yAxis);
    Serial.print(" | Mode: ");
    Serial.println(data.autoMode ? "Auto" : "Manual");
  } else {
    Serial.println("Failed to send data");
  }

  delay(100); // Delay for stability
}