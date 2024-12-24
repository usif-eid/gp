#include <Wire.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <TinyGPS++.h>
#include <Kalman.h>  // Include the Kalman library

// MPU6050 I2C address
const int MPU_addr = 0x68;

// Raw accelerometer data
int16_t AcX, AcY, AcZ;

// Tilt angles
double x, y, z;
double rawX, rawY, rawZ; // Raw tilt angles before Kalman filter

// Kalman filter setup
Kalman kalmanX; // Kalman filter for X-axis
Kalman kalmanY; // Kalman filter for Y-axis
Kalman kalmanZ; // Kalman filter for Z-axis

// GPS setup
TinyGPSPlus gps;
SoftwareSerial SerialGPS(D6, D7); // RX, TX

// Wi-Fi credentials
const char* ssid = "Adel Hassan";
const char* password = "Adel283645";

// GPS variables
float Latitude, Longitude;
int year, month, date, hour, minute, second;
String DateString, TimeString, LatitudeString, LongitudeString;

// Web server
WiFiServer server(80);

void setup() {
  // Initialize Serial Monitor and I2C
  Serial.begin(9600);
  Wire.begin();
  
  // Initialize MPU6050
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B); // Power management register
  Wire.write(0);    // Wake MPU6050
  Wire.endTransmission(true);

  // Initialize GPS
  SerialGPS.begin(9600);

  // Connect to Wi-Fi
  Serial.println();
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Wi-Fi connected");
  server.begin();
  Serial.println("Server started");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Read raw accelerometer data from MPU6050
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B); // Starting register for accelerometer data
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr, 14, true);
  AcX = Wire.read() << 8 | Wire.read();
  AcY = Wire.read() << 8 | Wire.read();
  AcZ = Wire.read() << 8 | Wire.read();

  // Calculate raw tilt angles (before Kalman filter)
  rawX = atan(AcY / sqrt(pow(AcX, 2) + pow(AcZ, 2))) * 180 / PI;
  rawY = atan(-AcX / sqrt(pow(AcY, 2) + pow(AcZ, 2))) * 180 / PI;
  rawZ = atan(sqrt(pow(AcX, 2) + pow(AcY, 2)) / AcZ) * 180 / PI;

  // Gyroscope rates (converted to degrees/sec)
  double gyroXrate = AcX / 131.0;
  double gyroYrate = AcY / 131.0;
  double gyroZrate = AcZ / 131.0;

  // Apply Kalman filter
  x = kalmanX.getAngle(rawX, gyroXrate, 0.01); // Replace 0.01 with your loop time (dt)
  y = kalmanY.getAngle(rawY, gyroYrate, 0.01);
  z = kalmanZ.getAngle(rawZ, gyroZrate, 0.01); // Apply Kalman filter for Z-axis

  // Process GPS data
  while (SerialGPS.available() > 0) {
    if (gps.encode(SerialGPS.read())) {
      if (gps.location.isValid()) {
        Latitude = gps.location.lat();
        LatitudeString = String(Latitude, 6);
        Longitude = gps.location.lng();
        LongitudeString = String(Longitude, 6);
      }

      if (gps.date.isValid()) {
        DateString = "";
        date = gps.date.day();
        month = gps.date.month();
        year = gps.date.year();
        if (date < 10) DateString += '0';
        DateString += String(date) + "/";
        if (month < 10) DateString += '0';
        DateString += String(month) + "/";
        DateString += String(year);
      }

      if (gps.time.isValid()) {
        TimeString = "";
        hour = gps.time.hour() + 2; // Adjust UTC
        minute = gps.time.minute();
        second = gps.time.second();
        if (hour < 10) TimeString += '0';
        TimeString += String(hour) + ":";
        if (minute < 10) TimeString += '0';
        TimeString += String(minute) + ":";
        if (second < 10) TimeString += '0';
        TimeString += String(second);
      }
    }
  }

  // Serve the web page
  WiFiClient client = server.available();
  if (!client) return;

  // HTTP Response
  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE html><html><head><title>Sensor Readings</title>";
  s += "<style>table, th, td {border: 1px solid blue; text-align: center;} body {font-family: Arial;}</style></head><body>";
  s += "<h1 style='text-align: center;'>MPU6050 and GPS Sensor Readings</h1>";
  s += "<table style='width: 80%; margin: auto;'><tr><th>Parameter</th><th>Raw Value</th><th>Filtered Value</th></tr>";
  s += "<tr><td>Angle X (Raw)</td><td>" + String(rawX, 2) + "</td><td>" + String(x, 2) + "</td></tr>";
  s += "<tr><td>Angle Y (Raw)</td><td>" + String(rawY, 2) + "</td><td>" + String(y, 2) + "</td></tr>";
  s += "<tr><td>Angle Z (Raw)</td><td>" + String(rawZ, 2) + "</td><td>" + String(z, 2) + "</td></tr>";
  s += "<tr><td>Latitude</td><td>" + LatitudeString + "</td><td></td></tr>";
  s += "<tr><td>Longitude</td><td>" + LongitudeString + "</td><td></td></tr>";
  s += "<tr><td>Date</td><td>" + DateString + "</td><td></td></tr>";
  s += "<tr><td>Time</td><td>" + TimeString + "</td><td></td></tr>";
  s += "</table>";

  if (gps.location.isValid()) {
    s += "<p style='text-align: center;'><a style='color: red; font-size: 120%;' href='http://maps.google.com/maps?q=";
    s += LatitudeString + "," + LongitudeString + "' target='_blank'>Open Location in Google Maps</a></p>";
  }

  s += "</body></html>";

  client.print(s);
}
