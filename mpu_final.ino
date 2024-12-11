#include <Wire.h>

// MPU6050 I2C address
const int MPU_addr = 0x68;

// Raw data variables
int16_t AcX, AcY, AcZ;

// Tilt angles
double x, y, z;

void setup() {
  // Initialize I2C communication
  Wire.begin(); 
  Serial.begin(9600); // Set baud rate for Serial Monitor

  // Wake up MPU6050
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B); // Power management register
  Wire.write(0);    // Wake MPU6050
  Wire.endTransmission(true);

  Serial.println("MPU6050 Initialized. Move the module to see tilt angles.");
}

void loop() {
  // Read raw accelerometer data
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B); // Starting register for accelerometer data
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr, 14, true);

  AcX = Wire.read() << 8 | Wire.read();
  AcY = Wire.read() << 8 | Wire.read();
  AcZ = Wire.read() << 8 | Wire.read();

  // Calculate tilt angles
  x = atan(AcY / sqrt(pow(AcX, 2) + pow(AcZ, 2))) * 180 / PI;
  y = atan(-AcX / sqrt(pow(AcY, 2) + pow(AcZ, 2))) * 180 / PI;
  z = atan(sqrt(pow(AcX, 2) + pow(AcY, 2)) / AcZ) * 180 / PI;

  // Print angles to Serial Monitor
  Serial.print("Angle X: ");
  Serial.println(x);
  Serial.print("Angle Y: ");
  Serial.println(y);
  Serial.print("Angle Z: ");
  Serial.println(z);
  Serial.println("-----------------------------------");

  delay(500); // Small delay for stability
}