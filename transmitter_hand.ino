// Gesture Controlled Robot - TRANSMITTER (Hand / sensor side)
// Compatible with receiver_car (same pipe, same int data[2] packet)
// Board: Arduino Uno or Nano

#include <SPI.h>        // SPI for nRF24L01
#include "RF24.h"       // nRF24L01 library
#include "Wire.h"       // I2C for MPU6050
#include "I2Cdev.h"     // MPU6050 helper library
#include "MPU6050.h"    // MPU6050 library

// ---------- MPU6050 (accelerometer + gyro, only accel is used) ----------
MPU6050 mpu;
int16_t ax, ay, az;
int16_t gx, gy, gz;

// ---------- Packet: data[0] = X (forward/backward), data[1] = Y (left/right) ----------
int data[2];

// ---------- Radio ----------
RF24 radio(8, 9);                          // CE = 8, CSN = 9
const uint64_t pipe = 0xE8E8F0F0E1LL;      // must match receiver

void setup(void) {
  Serial.begin(9600);
  Wire.begin();

  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 NOT connected! Check wiring (SDA=A4, SCL=A5)");
  }

  if (!radio.begin()) {
    Serial.println("nRF24L01 NOT responding! Check wiring (3.3V, CE=8, CSN=9)");
  }
  radio.openWritingPipe(pipe);
  radio.stopListening();                   // transmit mode

  Serial.println("Transmitter ready");
}

void loop(void) {
  // Read accelerometer (and gyro) values
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // Map tilt to the ranges the receiver expects
  data[0] = map(ax, -17000, 17000, 300, 400);   // X axis: ~350 when flat
  data[1] = map(ay, -17000, 17000, 100, 200);   // Y axis: ~150 when flat

  radio.write(data, sizeof(data));

  Serial.print("X axis data = ");
  Serial.println(data[0]);
  Serial.print("Y axis data = ");
  Serial.println(data[1]);

  delay(20);                               // ~50 packets per second
}
