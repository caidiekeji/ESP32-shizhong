#pragma once
// ===================== 项目配置（按需修改这里） =====================

// ---- WiFi 配网 ----
// 首次启动会创建热点 "ESP32-Clock-Setup"，手机连接后自动弹出配网页
#define WIFI_AP_NAME     "ESP32-Clock-Setup"
#define WIFI_AP_PASSWORD "12345678"

// ---- I2C 引脚（ESP32-S3 默认 8/9；经典 ESP32 用 21/22）----
#define I2C_SDA          8
#define I2C_SCL          9

// ---- 传感器 I2C 地址 ----
// AHT20 固定 0x38；BMP280 模块 SDO 接 GND 为 0x76，接 VCC 为 0x77
#define AHT20_ADDR       0x38
#define BMP280_ADDR      0x76

// ---- 时间 ----
#define TZ_INFO          "CST-8"   // 中国标准时间 UTC+8
#define NTP_SERVER1      "ntp.aliyun.com"
#define NTP_SERVER2      "cn.pool.ntp.org"

// ---- 和风天气 ----
// 注意：2024年9月之后注册的和风天气账号，控制台会分配专属 API Host
// （形如 abc123.re.qweatherapi.com），此时把下面两个 Host 改成控制台里的专属 Host
//
// QWEATHER_KEY 不写死在源码里（仓库公开），编译时注入：
//   本地编译：先设置环境变量  set QWEATHER_KEY=你的key  （或 setx 永久生效）
//   GitHub Actions：仓库 Settings → Secrets and variables → Actions 添加 QWEATHER_KEY
#ifndef QWEATHER_KEY
#define QWEATHER_KEY ""
#endif
#define QWEATHER_API_HOST  "devapi.qweather.com"   // 实时天气/预报
#define QWEATHER_GEO_HOST  "geoapi.qweather.com"   // 城市搜索
#define DEFAULT_CITY       "北京"                   // IP定位失败时的回退城市

// ---- 刷新周期 ----
#define WEATHER_UPDATE_MINUTES  20      // 天气刷新间隔（分钟）
#define SENSOR_UPDATE_MS        5000    // 传感器采集间隔（毫秒）

// ---- mDNS 访问名：烧录后浏览器访问 http://esp32-clock.local ----
#define HOSTNAME          "esp32-clock"
