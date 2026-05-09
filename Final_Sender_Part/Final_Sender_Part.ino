#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <MPU6050.h>

// ── Pin Definitionsvfor sensor ──────────────────────────────────────────
#define FLAME_PIN     34   // Flame sensor analog pin (ADC)
#define VIBRATION_PIN 25   // SW420 digital output pin

// ── MPU6050 Object ───────────────────────────────────────────
MPU6050 mpu;


//Lora pin
#define SS    18
#define RST   14
#define DIO0  26

#define SCK   5
#define MISO  19
#define MOSI  27


  bool flm = true;
  bool vib = true;
  bool gyro = true;

void setup() {
  Serial.begin(115200);

  // MPU6050 Init (ESP32 default I2C: SDA=21, SCL=22)
  Wire.begin(21, 22);
  mpu.initialize();

  if (mpu.testConnection()) {
    Serial.println("MPU6050 connected successfully!");
  } else {
    Serial.println("MPU6050 connection FAILED!");
  }

  // Flame & Vibration Pin Setup
  pinMode(FLAME_PIN, INPUT);
  pinMode(VIBRATION_PIN, INPUT);

  // Initialize SPI with custom pins
  SPI.begin(SCK, MISO, MOSI, SS);

  // Setup LoRa pins
  LoRa.setPins(SS, RST, DIO0);

  Serial.println("LoRa Initializing...");

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed!");
    while (1);
  }

  Serial.println("LoRa init succeeded.");

  Serial.println("─────────────────────────────────────────");
  delay(500);
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

  char MPU6050Data[200];   // Create character array (string MPU6050Data)


  float tiltAngle = atan2(sqrt(accelX * accelX + accelY * accelY), accelZ) * 180 / PI;

  const char* tiltStatus = (tiltAngle > 50)
                          ? "Fall"
                          : "Healthy";

  // Store all formatted text into the MPU6050Data
  snprintf(MPU6050Data, sizeof(MPU6050Data),
           "MPU6050:\n"
           "   Accel (g)  -> X: %.2f  Y: %.2f  Z: %.2f\n"
           "   Gyro (d/s) -> X: %.2f  Y: %.2f  Z: %.2f\n   %s\n",
           accelX, accelY, accelZ,
           gyroX, gyroY, gyroZ, tiltStatus);



  // ── 2. Flame Sensor Data ───────────────────────────────────
  int flameRaw = analogRead(FLAME_PIN);          // 0–4095 (12-bit ADC)
  // float flameVoltage = (flameRaw / 4095.0) * 3.3;
  // Lower raw value = more IR light = stronger flame detected
  const char* flameStatus = (flameRaw < 1500)
                          ? "FLAME DETECTED"
                          : "NO FLAME";



  char Flame_data[100];

  snprintf(Flame_data, sizeof(Flame_data),
           "Flame Sensor:\n"
           "   Status: %s\n",
           flameStatus);


  // ── 3. SW420 Vibration Sensor Data ────────────────────────
  int vibrationState = digitalRead(VIBRATION_PIN);
  const char* vibStatus = (vibrationState == HIGH)
                        ? "VIBRATION DETECTED"
                        : "NO VIBRATION";


  char Viberatio_data[100];

  snprintf(Viberatio_data, sizeof(Viberatio_data),
          "Vibration Sensor (SW420):\n"
          "   Status: %s\n",
          vibStatus);


  // if(((flameRaw < 1500) || (vibrationState == HIGH) || (tiltAngle > 50)) && (flm || vib || gyro)){

    String pac = "000";
    bool is_trigger = false;

    if(flameRaw < 1500 && flm) {pac[0] = '1'; flm = false; is_trigger = true;}
    if(vibrationState == HIGH && vib) {pac[1] = '1'; is_trigger = true;}
    if(tiltAngle > 50 && gyro) {pac[2] = '1'; gyro = vib = false; is_trigger = true;}
    

    if(is_trigger){

      Serial.print(MPU6050Data);
      Serial.print(Flame_data);
      Serial.print(Viberatio_data);

      LoRa.beginPacket();
      LoRa.print(pac);
      LoRa.endPacket();
    // }

  }else{
    Serial.println("Good");
  }

  int packetSize = LoRa.parsePacket();

  if (packetSize) {

    Serial.print("Received packet: ");

    // Read packet
    while (LoRa.available()) {
      String received = LoRa.readString();
      if(received[0] == '1') gyro = vib = flm = true;
      if(received[0] == '1') ESP.restart();
      // Serial.print(received); 
      Serial.println(received);
    }

  }

  // Open RX window immediately after TX
  unsigned long listenStart = millis();
  while (millis() - listenStart < 250) {   // 250ms listen window
    int pkt = LoRa.parsePacket();
    if (pkt) {
      String ack = "";
      while (LoRa.available()) ack += (char)LoRa.read();
      if (ack == "1") {
        // ACK received — buzzer off confirmed
        
        gyro = vib = flm = true;
        ESP.restart();
      }
    }
  }

  // delay(50);   // then repeat





  

  
  // Serial.println("Sending packet...");

  // // Print to Serial Monitor
  // Serial.print(MPU6050Data);
  // Serial.print(Flame_data);
  // Serial.print(Viberatio_data);


  // // Send via LoRa
  // LoRa.beginPacket();
  // LoRa.print(MPU6050Data);
  // LoRa.print(Flame_data);
  // LoRa.print(Viberatio_data);
  // LoRa.endPacket();


  delay(50);
}


// if (abs(gyroX) > 50 || abs(gyroY) > 50) {
//         Serial.println("VIBRATION DETECTED: Possible cutting!");
//     }

//     // 2. Detect Wind (Slower, rhythmic swaying)
//     else if (abs(accelX) > 0.15 || abs(accelY) > 0.15) {
//         Serial.println("WIND: Tree is swaying.");
//     }

//     // 3. Detect Falling (The 'Gravity' vector has shifted significantly)
//     // If the tree was vertical, Accel Z was ~1.0. If it falls, Z drops toward 0.
//     if (accelZ < 0.5) {
//         Serial.println("CRITICAL: TREE IS BEING CUT / FALLING!");
//     }