# smart-switch

用小度智能音箱语音开灯、关灯。

ESP32 驱动一个 SG90 舵机，模拟人手拨动墙壁开关；语音指令经小度音箱接入巴法云 MQTT 云，
ESP32 订阅主题后转动舵机到对应角度（关 30°、开 120°），并把当前状态回报给云端，
从而与米家 / 小爱生态联动。

## 工作链路

```
小度音箱 → 巴法云 MQTT → ESP32 → SG90 舵机 → 墙壁开关
                                 ↓
                          状态回报（on / off）
```

## 技术栈

- 硬件：ESP32 devkit、SG90 舵机、5V 独立供电（需与 ESP32 共地）
- 框架：Arduino (ESP32)
- 依赖：ESP32Servo、PubSubClient
- 构建：PlatformIO（`platformio.ini`，环境 `esp32dev`）
- 云端：巴法云 MQTT 设备云（bemfa.com:9501）

## 功能

- MQTT 下发 `on` / `off` 控制开关（兼容 `on#xxx` 格式）
- 状态去重：仅在状态真正变化时动作并上报，避免云端消息回环
- 上线自动同步当前开关状态
- WiFi / MQTT 断线自动重连（5 秒退避）
- 串口调试指令 `on` / `off`，便于现场排查

## 硬件接线

| 舵机 | ESP32 |
|------|-------|
| 信号线 | GPIO18 |
| 电源 | 外部 5V（不可由 ESP32 3V3 供电） |
| 地线 | 与 ESP32 共地 |

## 配置

复制 `include/secrets.h.example` 为 `include/secrets.h`，填入自己的 WiFi 账号与巴法云私钥、
主题名（该文件已在 `.gitignore` 中，不要提交）。

## 构建与烧录

```bash
pio run
pio run -t upload
pio device monitor
```
