#include <Wire.h>
#include <MPU6050.h>

// ── Pin Definitions ──────────────────────────────────────────
#define FLAME_PIN     34   // Flame sensor analog pin (ADC)
#define VIBRATION_PIN 25   // SW420 digital output pin

// ── MPU6050 Object ───────────────────────────────────────────
MPU6050 mpu;

void setup() {
  Serial.begin(115200);

  // MPU6050 Init (ESP32 default I2C: SDA=21, SCL=22)
  Wire.begin(21, 22);
  mpu.initialize();

  if (mpu.testConnection()) {
    Serial.println("✅ MPU6050 connected successfully!");
  } else {
    Serial.println("❌ MPU6050 connection FAILED!");
  }

  // Flame & Vibration Pin Setup
  pinMode(FLAME_PIN, INPUT);
  pinMode(VIBRATION_PIN, INPUT);

  Serial.println("─────────────────────────────────────────");
  delay(1000);
}

void loop() {
  // ── 1. MPU6050 Data ────────────────────────────────────────
  int16_t ax, ay, az;
  int16_t gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // Convert raw values to readable units
  float accelX = ax / 16384.0;  // ±2g range → LSB/g = 16384
  float accelY = ay / 16384.0;
  float accelZ = az / 16384.0;
  float gyroX  = gx / 131.0;   // ±250°/s range → LSB/(°/s) = 131
  float gyroY  = gy / 131.0;
  float gyroZ  = gz / 131.0;

  Serial.println("📐 MPU6050:");
  Serial.print("   Accel (g)  → X: "); Serial.print(accelX, 2);
  Serial.print("  Y: ");               Serial.print(accelY, 2);
  Serial.print("  Z: ");               Serial.println(accelZ, 2);
  Serial.print("   Gyro (°/s) → X: "); Serial.print(gyroX, 2);
  Serial.print("  Y: ");               Serial.print(gyroY, 2);
  Serial.print("  Z: ");               Serial.println(gyroZ, 2);

  // ── 2. Flame Sensor Data ───────────────────────────────────
  int flameRaw = analogRead(FLAME_PIN);          // 0–4095 (12-bit ADC)
  float flameVoltage = (flameRaw / 4095.0) * 3.3;
  // Lower raw value = more IR light = stronger flame detected
  String flameStatus = (flameRaw < 1500) ? "🔥 FLAME DETECTED!" : "✅ No Flame";

  Serial.println("🔥 Flame Sensor:");
  Serial.print("   Raw: ");    Serial.print(flameRaw);
  Serial.print("  Voltage: "); Serial.print(flameVoltage, 2);
  Serial.print("V  Status: "); Serial.println(flameStatus);

  // ── 3. SW420 Vibration Sensor Data ────────────────────────
  int vibrationState = digitalRead(VIBRATION_PIN);
  String vibStatus = (vibrationState == HIGH) ? "📳 VIBRATION DETECTED!" : "😴 No Vibration";

  Serial.println("📳 Vibration Sensor (SW420):");
  Serial.print("   State: "); Serial.print(vibrationState);
  Serial.print("  Status: "); Serial.println(vibStatus);

  Serial.println("─────────────────────────────────────────");
  delay(500);  // Read every 500ms
}