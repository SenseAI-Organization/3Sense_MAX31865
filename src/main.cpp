#include <Arduino.h>
#include <Adafruit_MAX31865.h>

// Use software SPI with exposed pins on FireBeetle ESP32 V2.0
// D2=GPIO25, D3=GPIO26, D4=GPIO27, D7=GPIO16
#define KpinCs   25  // D2 - Chip Select
#define KpinMosi 26  // D3 - Data In (SDI)
#define KpinMiso 27  // D4 - Data Out (SDO)
#define KpinClk  16  // D7 - Clock

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
  // TX=GPIO1, RX=GPIO3 (exposed as TX/RX on FireBeetle)
  Serial1.begin(9600, SERIAL_8N1, 3, 1);  // baud, config, RX pin, TX pin
  delay(100);

  // Initialize the MAX31865 with 2-wire configuration
  // Change to MAX31865_3WIRE or MAX31865_4WIRE as needed
  thermo.begin(MAX31865_2WIRE);
}

void loop() {
  uint16_t rtd = thermo.readRTD();
  float ratio = rtd / 32768.0;
  float resistance = RREF * ratio;
  float temperature = thermo.temperature(RNOMINAL, RREF);
  uint8_t fault = thermo.readFault();

  // Send data to another device via Serial1 (TX/RX pins)
  // Format: TEMP:25.50,RTD:16384,RES:110.25,FAULT:0
  Serial1.print("TEMP:");
  Serial1.print(temperature, 2);
  Serial1.print(",RTD:");
  Serial1.print(rtd);
  Serial1.print(",RES:");
  Serial1.print(resistance, 2);
  Serial1.print(",FAULT:");
  Serial1.println(fault);

  // Debug output to USB Serial
  Serial.print("RTD value: ");
  Serial.println(rtd);
  
  Serial.print("Ratio = ");
  Serial.println(ratio, 8);
  
  Serial.print("Resistance = ");
  Serial.println(resistance, 8);
  
  Serial.print("Temperature = ");
  Serial.println(temperature);

  // Check and print any faults
  if (fault) {
    Serial.print("Fault 0x");
    Serial.println(fault, HEX);
    
    if (fault & MAX31865_FAULT_HIGHTHRESH) {
      Serial.println("RTD High Threshold");
    }
    if (fault & MAX31865_FAULT_LOWTHRESH) {
      Serial.println("RTD Low Threshold");
    }
    if (fault & MAX31865_FAULT_REFINLOW) {
      Serial.println("REFIN- > 0.85 x Bias");
    }
    if (fault & MAX31865_FAULT_REFINHIGH) {
      Serial.println("REFIN- < 0.85 x Bias - FORCE- open");
    }
    if (fault & MAX31865_FAULT_RTDINLOW) {
      Serial.println("RTDIN- < 0.85 x Bias - FORCE- open");
    }
    if (fault & MAX31865_FAULT_OVUV) {
      Serial.println("Under/Over voltage");
    }
    thermo.clearFault();
  }
  
  Serial.println();
  delay(1000);
}