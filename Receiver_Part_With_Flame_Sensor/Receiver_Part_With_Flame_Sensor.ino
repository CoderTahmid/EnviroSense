#include <SPI.h>
#include <LoRa.h>

#define LORA_SS    18
#define LORA_RST   14
#define LORA_DIO0  26

void setup() {
  Serial.begin(115200);
  delay(1000); // Wait for Serial to stabilize

  Serial.println("\n\n===== LoRa Receiver Debug =====");
  Serial.println("Step 1: Serial OK ✅");

  // Init SPI
  SPI.begin(5, 19, 27, 18);
  Serial.println("Step 2: SPI Started ✅");

  // Init LoRa pins
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  Serial.println("Step 3: LoRa Pins Set ✅");

  // Try LoRa begin
  Serial.println("Step 4: Starting LoRa on 433 MHz...");
  int retry = 0;
  while (!LoRa.begin(433E6)) {
    Serial.print("❌ LoRa init failed! Retrying... ");
    Serial.println(retry++);
    delay(1000);
    if (retry > 5) {
      Serial.println("🚫 LoRa failed after 5 retries.");
      Serial.println("👉 Check: VCC=3.3V, wiring, antenna attached?");
      while (true);
    }
  }

  Serial.println("Step 4: LoRa Init ✅");

  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);

  Serial.println("Step 5: LoRa Settings Applied ✅");
  Serial.println("===============================");
  Serial.println("✅ Receiver Ready! Listening...\n");
}

void loop() {
  int packetSize = LoRa.parsePacket();

  if (packetSize) {
    String received = "";
    while (LoRa.available()) {
      received += (char)LoRa.read();
    }

    int rssi = LoRa.packetRssi();
    float snr = LoRa.packetSnr();

    Serial.println("=============================");
    Serial.print("📦 Received: ");
    Serial.println(received);
    Serial.print("📶 RSSI: ");
    Serial.print(rssi);
    Serial.println(" dBm");
    Serial.print("🔊 SNR:  ");
    Serial.print(snr);
    Serial.println(" dB");

    if (received.indexOf("FLAME_DETECTED") != -1) {
      Serial.println("🔥 ALERT! FLAME DETECTED!");
    } else if (received.indexOf("NO_FLAME") != -1) {
      Serial.println("✅ No Flame");
    }

    int analogIdx = received.indexOf("ANALOG:") + 7;
    if (analogIdx > 7) {
      Serial.print("📊 Analog Value: ");
      Serial.println(received.substring(analogIdx).toInt());
    }
    Serial.println("=============================\n");

  } else {
    // Heartbeat every 3 seconds so you know it's alive
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 3000) {
      Serial.println("👂 Listening... (no packet yet)");
      lastPrint = millis();
    }
  }
}