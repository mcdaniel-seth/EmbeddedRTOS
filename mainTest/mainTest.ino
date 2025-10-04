void setup() {
  // put your setup code here, to run once:
    Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:
    Serial.print("Hello, World!\n");
    int a = 6;
    int b = 7;
    Serial.printf("SIX %d SEVEN %d\n", a, b);
    delay(1000);
}
