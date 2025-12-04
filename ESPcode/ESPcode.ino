//  ESP32 code to read data from STM32 over UART and send to Blynk app
 
#define BLYNK_TEMPLATE_ID   "TMPL2z_ZeGRMJ"         // template ID
#define BLYNK_TEMPLATE_NAME "retryled"              // template name
#define BLYNK_AUTH_TOKEN    "O4gpJ9Uk9cL95nwtpw4-by20ntx9s7xJ"   // device token
 
#define BLYNK_PRINT Serial
 
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <HardwareSerial.h>
 
// WiFi 
char ssid[] = "esp32phone";     // your SSID
char pass[] = "mock.pass";      // your password
 
// UART
HardwareSerial& ESPUART = Serial2;    // UART2
 
const int RX2_PIN = 16;   // from STM32 PA9 (TX)
const int TX2_PIN = 17;   // to STM32 PA10 (RX)
const int LED_PIN = 2;    // onboard LED
 
float current_mA = 0.0f;  // latest current from STM32 in mA
 
void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
 
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("=== ESP32 UART + Blynk Bridge ===");
  Serial.println("Using token: " + String(BLYNK_AUTH_TOKEN));
 
  // flash indicator LED a few times
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(150);
    digitalWrite(LED_PIN, LOW);
    delay(150);
  }
 
  // connect to WiFi 
  Serial.println("Connecting to Blynk...");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  Serial.println("Blynk.begin() returned (device should be online)");
 
  // start UART2 
  ESPUART.begin(115200, SERIAL_8N1, RX2_PIN, TX2_PIN);
  Serial.println("UART2 initialized (RX2=" + String(RX2_PIN) +
                 ", TX2=" + String(TX2_PIN) + ")");
}
 
void loop() {
  Blynk.run();   // keep Blynk connection going
 
  static String line;
 
  // Read from STM32 
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
        current_mA = line.substring(2).toFloat();
 
        Serial.print("Current from STM32: ");
        Serial.print(current_mA, 1);
        Serial.println(" mA");
 
        // send current to Blynk gauge on V1
        Serial.print("Sending to Blynk V1: ");
        Serial.println(current_mA, 1);
        Blynk.virtualWrite(V1, current_mA);
      }
      else {
        // Anything else
        Serial.print("Raw RX: ");
        Serial.println(line);
      }
 
      // clear for next line
      line = "";
    }
    else if (c != '\r') {
      // grab characters except ne wline
      line += c;
    }
  }
}