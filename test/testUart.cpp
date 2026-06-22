#include <Arduino.h>

// Pin to read (analog pin example)
#define INPUT_PIN 36  // GPIO36 (A0 on FireBeetle ESP32)

// UART1 pins for communication with ESP32-S3
// Use GPIO16 (RX) and GPIO17 (TX) - these support UART
#define rxPin 16  // GPIO16 RX - connected to TX of ESP32-S3
#define txPin 17  // GPIO17 TX - connected to RX of ESP32-S3


void setup() {
  // Initialize USB Serial (UART0) for CoolTerm monitoring
  Serial.begin(115200);
  
  // Initialize UART1 for ESP32-S3 communication
  Serial1.begin(115200, SERIAL_8N1, rxPin, txPin);
  
  // Configure input pin
  pinMode(INPUT_PIN, INPUT);
  
  delay(100);
  
  Serial.println("ESP32 UART Test - Starting...");
  Serial.println("Sending data to ESP32-S3 via GPIO16(RX)/17(TX)");
}

void loop() {
  // Read pin value
  int pinValue = analogRead(INPUT_PIN);  // For analog: 0-4095
  // For digital pin use: int pinValue = digitalRead(INPUT_PIN);  // 0 or 1
  
  // Send value via USB Serial (to CoolTerm on PC)
  Serial.print("PIN:");
  Serial.println(pinValue);
  
  // Send value via UART1 (to ESP32-S3)
  Serial1.print("PIN:");
  Serial1.println(pinValue);
  
  delay(100);  // Send every 100ms
}
