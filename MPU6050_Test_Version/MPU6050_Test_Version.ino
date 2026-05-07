#include <Wire.h>
#define MPU_ADDR 0x68

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  // Wake up MPU6050 (exits sleep mode)
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); // PWR_MGMT_1
  Wire.write(0x00); // 0 = wake up
  Wire.endTransmission(true);

  Serial.println("MPU6050 ready");
}

void loop() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); // Start at ACCEL_XOUT_H
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 14, true); // 14 bytes: accel + temp + gyro

  int16_t ax = Wire.read() << 8 | Wire.read();
  int16_t ay = Wire.read() << 8 | Wire.read();
  int16_t az = Wire.read() << 8 | Wire.read();
  int16_t tmp = Wire.read() << 8 | Wire.read();
  int16_t gx = Wire.read() << 8 | Wire.read();
  int16_t gy = Wire.read() << 8 | Wire.read();
  int16_t gz = Wire.read() << 8 | Wire.read();

  Serial.printf("Ax:%.2f Ay:%.2f Az:%.2f | Gx:%.1f Gy:%.1f Gz:%.1f | T:%.1f°C\n",
    ax/16384.0, ay/16384.0, az/16384.0,
    gx/131.0,  gy/131.0,  gz/131.0,
    tmp/340.0 + 36.53);

  delay(200);
}