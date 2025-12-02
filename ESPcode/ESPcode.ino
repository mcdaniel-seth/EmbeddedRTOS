#include <HardwareSerial.h>

HardwareSerial& ESPUART = Serial2;  // UART2

const int RX2_PIN = 16;   // from STM32 PA9 (TX)
const int TX2_PIN = 17;   // to   STM32 PA10 (RX) 
const int LED_PIN = 2;   

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.begin(115200); 
  delay(1000);
  Serial.println();
  Serial.println("=== ESP32 UART Bridge ===");
  Serial.println("waiting for STM32 data...");

  // flash indicator
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(150);
    digitalWrite(LED_PIN, LOW);
    delay(150);
  }

  // Start UART2
  ESPUART.begin(115200, SERIAL_8N1, RX2_PIN, TX2_PIN);
  Serial.println("UART2 initialized (RX2=" + String(RX2_PIN) + ", TX2=" + String(TX2_PIN) + ")");
}

void loop() {
  static String line;

  // read from STM32 
  while (ESPUART.available()) {
    char c = (char)ESPUART.read();

    if (c == '\n') {
      // complete line received
      if (line.startsWith("T=")) {
        float tempF = line.substring(2).toFloat();
        Serial.print("Temp from STM32: ");
        Serial.print(tempF, 1);
        Serial.println(" F");
      }
      else if (line.startsWith("I=")) {
        float current_mA = line.substring(2).toFloat();
        Serial.print("Current from STM32: ");
        Serial.print(current_mA, 1);
        Serial.println(" mA");
      }
      else {
        Serial.print("Raw RX: ");
        Serial.println(line);
      }
      line = "";   // reset buffer 
    } 
    else if (c != '\r') {
      line += c;
    }
  }
}
