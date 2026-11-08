#pragma once
#include <Arduino.h>

// 0.91" SSD1306 128x32 I2C OLED（与传感器共用 I2C 总线）
// 自动检测地址 0x3C/0x3D，未接屏幕时所有函数静默跳过

bool displayInit();                                   // 探测并初始化，返回是否找到屏幕
bool displayOK();
void displayShowMsg(const char *l1, const char *l2);  // 显示两行临时消息（启动/配网时用）
void displayLoop();                                   // 主循环里调用，每秒刷新，时钟页/信息页交替
