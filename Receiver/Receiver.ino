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

  // Initialize SPI
  SPI.begin(SCK, MISO, MOSI, SS);

  // Set LoRa pins
  LoRa.setPins(SS, RST, DIO0);

  Serial.println("LoRa Receiver Initializing...");

  // Start LoRa
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed!");
    while (1);
  }

  Serial.println("LoRa Receiver Ready");
}

void loop() {

  // Check if packet received
  int packetSize = LoRa.parsePacket();

  if (packetSize) {

    Serial.print("Received packet: ");

    // Read packet
    while (LoRa.available()) {
      String received = LoRa.readString();
      Serial.print(received); 
    }

    // Print RSSI (signal strength)
    Serial.print("  | RSSI: ");
    Serial.println(LoRa.packetRssi());
  }
}