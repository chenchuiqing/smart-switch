#include <Arduino.h>
#include <ESP32Servo.h>
#include <strings.h>

// 信号线接 GPIO18；舵机电源由外部 5V 独立供电，且必须与 ESP32 共地
const int SERVO_PIN = 18;

const int ANGLE_OFF = 60;   // "关"位置
const int ANGLE_ON = 120;   // "开"位置

Servo wallSwitch;
int currentAngle = ANGLE_ON;  // 上电即在"开"位置

// 直接给目标角度，舵机以自身全速转动；SG90 额定 0.1s/60°，按 2ms/度 等待到位
static void moveTo(int target) {
  int delta = abs(target - currentAngle);
  currentAngle = target;
  wallSwitch.write(target);
  delay(delta * 2 + 50);
}

// 拨动开关：状态相同时直接忽略，避免反复顶住机械结构
static void setSwitch(bool on) {
  int target = on ? ANGLE_ON : ANGLE_OFF;
  if (currentAngle == target) {
    Serial.printf("already %s\n", on ? "on" : "off");
    return;
  }
  Serial.printf("switch %s: %d -> %d deg\n", on ? "on" : "off", currentAngle, target);
  moveTo(target);
  Serial.printf("hold at %d deg\n", target);
}

static void processCommand(const char* cmd) {
  if (strcasecmp(cmd, "on") == 0) {
    setSwitch(true);
  } else if (strcasecmp(cmd, "off") == 0) {
    setSwitch(false);
  } else {
    Serial.printf("unknown command: %s (use on / off)\n", cmd);
  }
}

// 非阻塞读取串口命令行，避免动态内存分配
static void pollSerial() {
  static char buf[16];
  static uint8_t len = 0;
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\r' || c == '\n') {
      if (len > 0) {
        buf[len] = '\0';
        processCommand(buf);
        len = 0;
      }
    } else if (len < sizeof(buf) - 1) {
      buf[len++] = c;
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // USB 转串口复位时有几百 ms 空窗，等它稳定
  Serial.println();
  Serial.println("=== smart switch ready ===");
  Serial.printf("signal pin : GPIO%d\n", SERVO_PIN);
  Serial.printf("angles      : off %d deg, on %d deg\n", ANGLE_OFF, ANGLE_ON);
  Serial.println("commands    : on / off");

  wallSwitch.attach(SERVO_PIN);
  wallSwitch.write(ANGLE_ON);  // 上电直接停在"开"位置
  delay(1000);
}

void loop() {
  pollSerial();
  delay(5);
}
