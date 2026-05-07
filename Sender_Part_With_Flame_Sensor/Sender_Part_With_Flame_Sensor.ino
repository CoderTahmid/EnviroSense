#include <SPI.h>
#include <LoRa.h>

// LoRa Pin Definitions (Your Wiring)
#define LORA_SS    18   // NSS  → GPIO 18
#define LORA_RST   14   // RST  → GPIO 14
#define LORA_DIO0  26   // DIO0 → GPIO 26

// SPI Pins (Your Wiring)
// SCK  → GPIO 5
// MISO → GPIO 19
// MOSI → GPIO 27

// Flame Sensor Pins
#define FLAME_DIGITAL_PIN  4
#define FLAME_ANALOG_PIN   34

int packetCount = 0;

void setup() {
  Serial.begin(115200);
  pinMode(FLAME_DIGITAL_PIN, INPUT);

  // Custom SPI pins
  SPI.begin(5, 19, 27, 18); // SCK, MISO, MOSI, SS

  // Initialize LoRa
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  Serial.println("Starting LoRa Transmitter...");

  if (!LoRa.begin(433E6)) {
    Serial.println("❌ LoRa init failed! Check wiring.");
    while (true);
  }

  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setTxPower(17);

  Serial.println("✅ LoRa Transmitter Ready!");
}

void loop() {
  int digitalVal = digitalRead(FLAME_DIGITAL_PIN);
  int analogVal  = analogRead(FLAME_ANALOG_PIN);

  String flameStatus = (digitalVal == LOW) ? "FLAME_DETECTED" : "NO_FLAME";

  LoRa.beginPacket();
  LoRa.print("PKT:");
  LoRa.print(packetCount);
  LoRa.print("|STATUS:");
  LoRa.print(flameStatus);
  LoRa.print("|ANALOG:");
  LoRa.print(analogVal);
  LoRa.endPacket();

  Serial.print("📤 Sent Packet #");
  Serial.print(packetCount);
  Serial.print(" | Status: ");
  Serial.print(flameStatus);
  Serial.print(" | Analog: ");
  Serial.println(analogVal);

  packetCount++;
  delay(2000);
}