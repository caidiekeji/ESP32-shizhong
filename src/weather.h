#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// 单日预报
struct ForecastDay {
    char date[12];   // 2026-11-09
    char text[24];   // 白天天气现象
    int  icon;       // 和风天气图标代码
    int  tmax;       // 最高温 ℃
    int  tmin;       // 最低温 ℃
};

// 天气数据（由天气任务写入，Web 线程加锁读取）
struct WeatherData {
    bool  valid = false;
    int   icon = 0;
    int   windScale = 0;
    float temp = 0, feels = 0, hum = 0, pressure = 0;
    char  text[24] = "";
    char  windDir[16] = "";
    time_t updated = 0;
    int   fcCount = 0;
    ForecastDay fc[3];
};

extern WeatherData g_wx;
extern SemaphoreHandle_t wxMutex;

void   weatherStart();                       // 创建天气刷新任务
String weatherGetCity();                     // 当前显示城市名（加锁）
bool   weatherIsAuto();                      // 是否处于 IP 自动定位模式
void   weatherSetCity(const String &city);   // 设置城市："auto"=IP自动定位，否则为具体城市名
String urlEncode(const String &s);           // UTF-8 百分号编码（城市名用）
