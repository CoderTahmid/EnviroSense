#include <SPI.h>
#include <LoRa.h>

#define SS    18
#define RST   14
#define DIO0  26

#define SCK   5
#define MISO  19
#define MOSI  27

void setup() {
  Serial.begin(115200);

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
}

void loop() {
  Serial.println("Sending packet...");

  LoRa.beginPacket();
  LoRa.print("My CGPA dropped this semester ToT");
  LoRa.endPacket();

  delay(2000);
}