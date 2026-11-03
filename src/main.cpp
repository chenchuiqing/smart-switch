#include <Arduino.h>
#include <ESP32Servo.h>

// 信号线接 GPIO18；舵机电源由外部 5V 独立供电，且必须与 ESP32 共地
const int SERVO_PIN = 18;

const int ANGLE_MIN = 0;    // 扫动下限
const int ANGLE_MAX = 180;  // 扫动上限
const int ANGLE_HOLD = 90;  // 上电与空闲时的中位角，避免撞向机械端

Servo wallSwitch;

// 从 from 匀速转到 to，每步停 stepDelayMs 毫秒
static void sweepTo(int from, int to, int stepDelayMs) {
  int step = (to >= from) ? 1 : -1;
  for (int angle = from; angle != to + step; angle += step) {
    wallSwitch.write(angle);
    if (angle % 30 == 0) {
      Serial.printf("angle: %d\n", angle);
    }
    delay(stepDelayMs);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // USB 转串口复位时有几百 ms 空窗，等它稳定
  Serial.println();
  Serial.println("=== servo sweep test ===");
  Serial.printf("signal pin : GPIO%d\n", SERVO_PIN);

  wallSwitch.attach(SERVO_PIN);
  wallSwitch.write(ANGLE_HOLD);  // 先在中位站稳，再开始扫动
  delay(1000);
  Serial.printf("hold at %d deg\n", ANGLE_HOLD);
}

void loop() {
  sweepTo(ANGLE_HOLD, ANGLE_MIN, 10);
  delay(800);
  sweepTo(ANGLE_MIN, ANGLE_MAX, 10);
  delay(800);
  sweepTo(ANGLE_MAX, ANGLE_HOLD, 10);
  delay(2000);
}
