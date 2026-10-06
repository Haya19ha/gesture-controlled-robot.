// Gesture Controlled Robot - RECEIVER (Car side)
// Compatible with the transmitter code (same pipe, same int data[2] packet)
// Board: Arduino Uno or Nano

#include <SPI.h>
#include "RF24.h"

// ---------- L298N motor driver pins ----------
const int enbA = 3;   // Right motor speed (PWM)  -> remove ENA jumper on L298N
const int enbB = 6;   // Left motor speed (PWM)   -> remove ENB jumper on L298N
const int IN1  = 2;   // Right motor
const int IN2  = 4;   // Right motor
const int IN3  = 5;   // Left motor
const int IN4  = 7;   // Left motor
// If a motor spins the wrong way, swap its two IN pin numbers above.

// ---------- Speeds (0-255) ----------
int RightSpd = 200;
int LeftSpd  = 250;   // adjust both until the car drives straight

// ---------- Gesture thresholds ----------
// Transmitter sends X around 350 and Y around 150 when the hand is flat.
const int X_CENTER  = 350;
const int Y_CENTER  = 150;
const int DEAD_ZONE = 10;    // same as original thresholds (340/360, 140/160)

// ---------- Safety ----------
const unsigned long SIGNAL_TIMEOUT = 500;  // ms: stop if no signal

// ---------- Radio ----------
int data[2];
RF24 radio(8, 9);                          // CE = 8, CSN = 9
const uint64_t pipe = 0xE8E8F0F0E1LL;      // must match transmitter

// ---------- State ----------
enum Move { STOP, FORWARD, BACKWARD, LEFT, RIGHT };
const char* moveNames[] = { "stop", "forward", "backward", "left", "right" };
Move currentMove = STOP;
unsigned long lastReceived = 0;

void setMotors(int in1, int in2, int in3, int in4, int spdR, int spdL) {
  digitalWrite(IN1, in1);
  digitalWrite(IN2, in2);
  digitalWrite(IN3, in3);
  digitalWrite(IN4, in4);
  analogWrite(enbA, spdR);
  analogWrite(enbB, spdL);
}

void applyMove(Move m) {
  switch (m) {
    case FORWARD:  setMotors(HIGH, LOW,  HIGH, LOW,  RightSpd, LeftSpd); break;
    case BACKWARD: setMotors(LOW,  HIGH, LOW,  HIGH, RightSpd, LeftSpd); break;
    case RIGHT:    setMotors(LOW,  HIGH, HIGH, LOW,  RightSpd, LeftSpd); break;
    case LEFT:     setMotors(HIGH, LOW,  LOW,  HIGH, RightSpd, LeftSpd); break;
    default:       setMotors(LOW,  LOW,  LOW,  LOW,  0, 0);              break;
  }
  if (m != currentMove) {          // print only when the movement changes
    currentMove = m;
    Serial.println(moveNames[m]);
  }
}

// One clear decision per packet: the stronger tilt wins.
Move decideMove(int x, int y) {
  int dx = x - X_CENTER;
  int dy = y - Y_CENTER;
  int adx = abs(dx);
  int ady = abs(dy);
  bool xActive = adx > DEAD_ZONE;
  bool yActive = ady > DEAD_ZONE;

  if (!xActive && !yActive) return STOP;
  if (xActive && (!yActive || adx >= ady)) {
    return (dx < 0) ? FORWARD : BACKWARD;   // X < 340 forward, X > 360 backward
  }
  return (dy > 0) ? RIGHT : LEFT;           // Y > 160 right,   Y < 140 left
}

void setup() {
  pinMode(enbA, OUTPUT);
  pinMode(enbB, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  setMotors(LOW, LOW, LOW, LOW, 0, 0);     // start stopped

  Serial.begin(9600);
  if (!radio.begin()) {
    Serial.println("nRF24L01 NOT responding! Check wiring (3.3V, CE=8, CSN=9)");
  }
  radio.openReadingPipe(1, pipe);
  radio.startListening();
  Serial.println("Receiver ready");
}

void loop() {
  if (radio.available()) {
    radio.read(data, sizeof(data));

    // Ignore corrupted packets (valid X ~254-446, Y ~54-246)
    if (data[0] >= 250 && data[0] <= 450 && data[1] >= 50 && data[1] <= 250) {
      lastReceived = millis();
      applyMove(decideMove(data[0], data[1]));
    }
  }

  // Failsafe: stop the car if the signal is lost
  if (millis() - lastReceived > SIGNAL_TIMEOUT) {
    applyMove(STOP);
  }
}
