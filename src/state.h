#pragma once
#include <Arduino.h>

// 主循环与显示模块共享的传感器数据
struct IndoorData {
    bool  ahtOK = false;   // AHT20 初始化成功
    bool  bmpOK = false;   // BMP280 初始化成功
    float t = NAN;         // 温度 ℃（优先 AHT20）
    float h = NAN;         // 湿度 %RH
    float p = NAN;         // 气压 hPa（BMP280）
};
extern IndoorData indoor;
