#pragma once
// ===================== 板载 RGB 状态灯（WS2812） =====================
// 常见 ESP32-S3 N16R8 开发板的彩灯在 GPIO48；
// 若你的板子灯不亮/颜色不对，改 config.h 里的 LED_PIN 即可。
//
// 状态含义：
//   红灯常亮 = 启动中 / 正在连 WiFi
//   红灯闪烁 = 配网模式（热点 ESP32-Clock-Setup 已开启）
//   绿灯常亮 = 联网成功，1 分钟后自动熄灭（不做常亮灯打扰）
//   红灯     = 运行中 WiFi 掉线（重连后恢复绿灯 → 再熄灭）

#include <Arduino.h>

void ledInit();        // 上电调用：红灯亮
void ledConnected();   // WiFi 连接成功：绿灯亮，开始计时
void ledLoop();        // loop 中调用：1 分钟后熄灯 / 掉线变红
