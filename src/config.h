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

// ---- 巴法云（接入米家/小爱同学，不需要则留空）----
// 注册 https://cloud.bemfa.com ，控制台首页复制"私钥"
// GitHub 编译：仓库 Secrets 添加 BEMFA_UID；本地编译：set BEMFA_UID=你的私钥
#ifndef BEMFA_UID
#define BEMFA_UID ""
#endif
#define BEMFA_HOST            "bemfa.com"
#define BEMFA_PORT            9501
// 温湿度传感器主题（MQTT设备云创建，尾号须为 004），上报格式 #温度#湿度#
#define BEMFA_TOPIC_SENSOR    "fl9nwZI7L004"
// 屏幕开关主题（尾号须为 001），留空则不启用"小爱同学，打开/关闭时钟屏幕"
#define BEMFA_TOPIC_SWITCH    ""
#define BEMFA_REPORT_MINUTES  5        // 温湿度上报间隔（分钟）

// ---- 板载 RGB 状态灯（WS2812）----
// N16R8 开发板一般在 GPIO48；灯不亮可试 38 / 21
#define LED_PIN           48
#define LED_BRIGHTNESS    20      // 亮度 0-255（彩灯很亮，别调太高）
#define LED_ON_MS         60000   // 绿灯亮多久后熄灭（毫秒）

// ---- mDNS 访问名：烧录后浏览器访问 http://esp32-clock.local ----
#define HOSTNAME          "esp32-clock"
