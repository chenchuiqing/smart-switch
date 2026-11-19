#include <Arduino.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <strings.h>

#include "secrets.h"

// 信号线接 GPIO18；舵机电源由外部 5V 独立供电，且必须与 ESP32 共地
const int SERVO_PIN = 18;

const int ANGLE_OFF = 50;    // "关"位置 越小越靠里面
const int ANGLE_REST = 85;   // 拨完关灯后的归位角度（介于开/关之间），松开开关，不阻碍手动操作

// 巴法云 MQTT 设备云：明文端口 9501，clientID 必须填私钥
const char* MQTT_HOST = "bemfa.com";
const uint16_t MQTT_PORT = 9501;

Servo wallSwitch;
int currentAngle = ANGLE_REST;

// 巴法云会把设备 publish 的消息回推给订阅者（也就是设备自己），
// 用时间窗丢弃这条回声，避免同一条"off"被反复执行。
const uint32_t ECHO_GUARD_MS = 3000;
uint32_t suppressUntilMs = 0;

WiFiClient netClient;
PubSubClient mqtt(netClient);

// 直接给目标角度，舵机以自身全速转动；SG90 额定 0.1s/60°，按 2ms/度 等待到位
static void moveTo(int target) {
  int delta = abs(target - currentAngle);
  currentAngle = target;
  wallSwitch.write(target);
  delay(delta * 2 + 50);
}

// 只做关灯：先拨到关位，再退回待机角度，让开关松手，用户仍可手动开/关
static void turnOff() {
  Serial.printf("off: %d -> %d deg\n", currentAngle, ANGLE_OFF);
  moveTo(ANGLE_OFF);
  moveTo(ANGLE_REST);

  suppressUntilMs = millis() + ECHO_GUARD_MS;
  mqtt.publish(BEMFA_TOPIC, "off");  // 上报状态给小爱/米家
}

// 巴法云下发内容形如 "on" / "off" / "on#xx"，只取 # 之前的部分
static void onMqttMessage(char* topic, byte* payload, unsigned int len) {
  char msg[16];
  unsigned int n = len < sizeof(msg) - 1 ? len : sizeof(msg) - 1;
  memcpy(msg, payload, n);
  msg[n] = '\0';

  char* extra = strchr(msg, '#');
  if (extra != nullptr) {
    *extra = '\0';
  }
  Serial.printf("recv: %s\n", msg);

  if (strcasecmp(msg, "off") == 0) {
    if (millis() < suppressUntilMs) {  // 自己上报的回声，丢弃
      Serial.println("echo ignored");
      return;
    }
    turnOff();
  } else {
    Serial.printf("ignored msg: %s (only off supported)\n", msg);
  }
}

static void ensureConnected() {
  if (WiFi.status() != WL_CONNECTED || mqtt.connected()) {
    return;
  }

  static uint32_t lastTryMs = 0;
  if (millis() - lastTryMs < 5000) {  // 失败后 5 秒再试，避免疯狂重连
    return;
  }
  lastTryMs = millis();

  Serial.print("mqtt connecting ... ");
  if (mqtt.connect(BEMFA_UID)) {
    mqtt.subscribe(BEMFA_TOPIC);
    Serial.print("subscribed ");
    Serial.println(BEMFA_TOPIC);
    // 开关可被用户手动拨动，上电时真实状态未知，因此不做状态同步上报
  } else {
    Serial.printf("failed, rc=%d\n", mqtt.state());
  }
}

// 串口调试指令，云平台接好后仍保留，方便现场排查
static void processCommand(const char* cmd) {
  if (strcasecmp(cmd, "off") == 0) {
    turnOff();
  } else {
    Serial.printf("ignored command: %s (only off supported)\n", cmd);
  }
}

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

static void connectWiFi() {
  Serial.printf("wifi connecting to %s ", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  uint32_t startMs = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startMs < 20000) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("wifi ok, ip ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("wifi failed, will retry in loop");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // USB 转串口复位时有几百 ms 空窗，等它稳定
  Serial.println();
  Serial.println("=== smart switch ready ===");
  Serial.printf("signal pin : GPIO%d\n", SERVO_PIN);
  Serial.printf("angles     : off %d deg, rest %d deg\n", ANGLE_OFF, ANGLE_REST);
  Serial.println("commands   : off");

  wallSwitch.attach(SERVO_PIN);
  wallSwitch.write(ANGLE_REST);  // 上电停在待机角度，不压住开关

  connectWiFi();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMqttMessage);
  mqtt.setKeepAlive(30);  // 巴法云要求 30 秒内有心跳
}

void loop() {
  ensureConnected();
  mqtt.loop();
  pollSerial();
  delay(5);
}
