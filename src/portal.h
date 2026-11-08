#pragma once
// ===================== 中文配网门户（替代 WiFiManager） =====================
// 无已保存 WiFi 或连接失败时：
//   1. 开启热点 ESP32-Clock-Setup（密码 12345678）
//   2. 手机连接后自动弹出配网页（或浏览器访问 http://192.168.4.1）
//   3. 页面与时钟主页同款玻璃拟态深色风格，全中文
// 凭证保存在 Flash（NVS），下次开机直连

#include <Arduino.h>

// 尝试用已保存凭证连 WiFi；失败则启动配网门户（阻塞直到配网成功）
// 返回 true = 已联网
bool portalEnsureConnected();
