#include <Arduino.h>
#include <Adafruit_MAX31865.h>

// Use software SPI with exposed pins on FireBeetle ESP32 V2.0
// D2=GPIO25, D3=GPIO26, D4=GPIO27, D7=GPIO16
#define KpinCs   13  // D7 - Chip Select
#define KpinMosi 25  // D2 - Data In (SDI)
#define KpinMiso 26  // D3 - Data Out (SDO)
#define KpinClk  27  // D4 - Clock

#define rxPin 16  // GPIO16 RX - connected to TX of ESP32-S3
#define txPin 17  // GPIO17 TX - connected to RX of ESP32-S3


// Software SPI constructor: CS, MOSI, MISO, CLK
Adafruit_MAX31865 thermo = Adafruit_MAX31865(KpinCs, KpinMosi, 
                                             KpinMiso, KpinClk);

// The value of the Rref resistor for PT100
#define RREF      430.0
// The 'nominal' 0-degrees-C resistance of PT100
#define RNOMINAL  100.0

void setup() {
  // Serial for USB debugging
  Serial.begin(115200);
  delay(1000);
  Serial.println("Adafruit MAX31865 PT100 Sensor Test - ESP32");

  // Serial1 for communication with another device (TX/RX pins)
  Serial1.begin(115200, SERIAL_8N1, rxPin, txPin);  // baud, config, RX pin, TX pin
  delay(100);

  // Initialize the MAX31865 with 3-wire configuration
  // Change to MAX31865_2WIRE or MAX31865_4WIRE as needed
  thermo.begin(MAX31865_3WIRE);
}

void loop() {
  uint16_t rtd = thermo.readRTD();
  float ratio = rtd / 32768.0;
  float resistance = RREF * ratio;
  float temperature = thermo.temperature(RNOMINAL, RREF);
  uint8_t fault = thermo.readFault();

  // Send data to another device via Serial1 (TX/RX pins)
  Serial1.print(temperature, 2);

  // Debug output to USB Serial
  Serial.println(temperature, 2);

  delay(1000);
}