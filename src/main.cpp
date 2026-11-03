#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(1000);                       // USB 转串口复位时有几百 ms 空窗，等它稳定
  Serial.println();
  Serial.println("=== boot ok ===");
  Serial.printf("chip  : %s\n", ESP.getChipModel());
  Serial.printf("cores : %d\n", ESP.getChipCores());
  Serial.printf("flash : %lu KB\n", ESP.getFlashChipSize() / 1024);
  Serial.printf("heap  : %lu B\n", ESP.getFreeHeap());
}

void loop() {
  static unsigned long last = 0;
  if (millis() - last >= 2000) {
    last = millis();
    Serial.printf("alive %lu s, heap %lu\n", millis() / 1000, ESP.getFreeHeap());
  }
}