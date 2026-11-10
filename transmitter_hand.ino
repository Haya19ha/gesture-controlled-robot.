#include <Wire.h>
#include "I2Cdev.h"
#include "MPU6050.h"

MPU6050 mpu;
int16_t ax, ay, az;

const int TILT = 6000;
char command = 'S';

void setup() {
  Serial.begin(9600);
  Wire.begin();
  mpu.initialize();
}

void loop() {
  mpu.getAcceleration(&ax, &ay, &az);

  if      (ax < -TILT) command = 'F';
  else if (ax >  TILT) command = 'B';
  else if (ay >  TILT) command = 'R';
  else if (ay < -TILT) command = 'L';
  else                 command = 'S';

  Serial.write(command);
  delay(50);
}
