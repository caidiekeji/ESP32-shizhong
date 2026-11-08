#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Wire.h>
#include <time.h>
#include <ArduinoJson.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include <WiFiManager.h>

#include "config.h"
#include "state.h"
#include "display.h"
#include "led.h"
#include "weather.h"
#include "bemfa.h"
#include "web_pages.h"

WebServer server(80);
Preferences prefs;
Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp(&Wire);

// ---------------- 室内传感器数据（定义见 state.h） ----------------
IndoorData indoor;

static unsigned long lastSensorMs = 0;

// ---------------- 传感器采集 ----------------
static void readSensors() {
    if (indoor.ahtOK) {
        sensors_event_t hum, temp;
        if (aht.getEvent(&hum, &temp)) {
            indoor.t = temp.temperature;
            indoor.h = hum.relative_humidity;
        }
    } else if (indoor.bmpOK) {
        indoor.t = bmp.readTemperature();   // AHT20 不可用时退回 BMP280 温度
    }
    if (indoor.bmpOK) {
        float pa = bmp.readPressure();
        if (pa > 0) indoor.p = pa / 100.0f; // Pa -> hPa
    }
}

// ---------------- API: /api/data ----------------
static float r1(float v) { return isnan(v) ? v : roundf(v * 10) / 10; }

static void handleApiData() {
    JsonDocument doc;

    time_t now = time(nullptr);
    doc["time"] = (uint32_t)now;
    doc["city"] = weatherGetCity();
    doc["autoLoc"] = weatherIsAuto();

    if (xSemaphoreTake(wxMutex, pdMS_TO_TICKS(500))) {
        JsonObject wx = doc["wx"].to<JsonObject>();
        wx["valid"] = g_wx.valid;
        if (g_wx.valid) {
            wx["text"]   = g_wx.text;
            wx["icon"]   = g_wx.icon;
            wx["temp"]   = r1(g_wx.temp);
            wx["feels"]  = r1(g_wx.feels);
            wx["hum"]    = (int)roundf(g_wx.hum);
            String wind  = String(g_wx.windDir) + " " + g_wx.windScale + "级";
            wx["wind"]   = wind;
            wx["p"]      = (int)roundf(g_wx.pressure);
            wx["updated"]= (uint32_t)g_wx.updated;
        }
        JsonArray fc = doc["fc"].to<JsonArray>();
        for (int i = 0; i < g_wx.fcCount; i++) {
            JsonObject o = fc.add<JsonObject>();
            o["date"] = g_wx.fc[i].date;
            o["text"] = g_wx.fc[i].text;
            o["icon"] = g_wx.fc[i].icon;
            o["max"]  = g_wx.fc[i].tmax;
            o["min"]  = g_wx.fc[i].tmin;
        }
        xSemaphoreGive(wxMutex);
    }

    JsonObject in = doc["indoor"].to<JsonObject>();
    in["t"]   = r1(indoor.t);
    in["h"]   = r1(indoor.h);
    in["p"]   = r1(indoor.p);
    in["aht"] = indoor.ahtOK;
    in["bmp"] = indoor.bmpOK;

    JsonObject sys = doc["sys"].to<JsonObject>();
    sys["heap"] = (uint32_t)ESP.getFreeHeap();
    sys["rssi"] = (int)WiFi.RSSI();
    sys["up"]   = (uint32_t)(millis() / 1000);

    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
}

// ---------------- API: /api/setcity ----------------
// city=auto 开启 IP 自动定位；其他值设为手动城市
static void handleSetCity() {
    if (server.hasArg("city")) {
        String c = server.arg("city");
        c.trim();
        if (c.length() > 0 && c.length() < 32) {
            String saved = (c == "auto") ? "auto" : c;
            prefs.putString("city", saved);
            weatherSetCity(c);
            Serial.printf("[设置] 城市: %s (%s)\n", weatherGetCity().c_str(),
                          weatherIsAuto() ? "IP自动定位" : "手动");
            server.send(200, "application/json", "{\"ok\":true}");
            return;
        }
    }
    server.send(400, "application/json", "{\"ok\":false}");
}

// ---------------- 初始化 ----------------
void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("\n=== ESP32-S3 智能时钟启动 ===");

    // 状态灯：红灯亮（启动中）
    ledInit();

    // 持久化设置（"auto" = IP 自动定位，恢复动作在 weatherStart 之后）
    prefs.begin("clock", false);
    String c = prefs.getString("city", "auto");

    // 传感器
    Wire.begin(I2C_SDA, I2C_SCL);
    indoor.ahtOK = aht.begin(&Wire);
    indoor.bmpOK = bmp.begin(BMP280_ADDR);
    if (indoor.bmpOK) {
        bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                        Adafruit_BMP280::SAMPLING_X1,   // 温度过采样
                        Adafruit_BMP280::SAMPLING_X8,   // 气压过采样
                        Adafruit_BMP280::FILTER_X16,
                        Adafruit_BMP280::STANDBY_MS_500);
    }
    Serial.printf("[传感器] AHT20: %s | BMP280: %s\n",
                  indoor.ahtOK ? "OK" : "未找到",
                  indoor.bmpOK ? "OK" : "未找到");

    // 屏幕（0.91" SSD1306 128x32，共用 I2C 总线，未接则自动跳过）
    displayInit();

    // WiFi（首次启动自动进入配网热点模式）
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(HOSTNAME);
    WiFiManager wm;
    wm.setConfigPortalTimeout(180);
    displayShowMsg("WiFi 配网中…", "热点:ESP32-Clock-Setup");
    if (!wm.autoConnect(WIFI_AP_NAME, WIFI_AP_PASSWORD)) {
        Serial.println("[WiFi] 配网超时，重启重试");
        ESP.restart();
    }
    WiFi.setAutoReconnect(true);
    ledConnected();   // 绿灯：联网成功，LED_ON_MS 后自动熄灭
    Serial.printf("[WiFi] 已连接 %s，IP: %s\n",
                  WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
    displayShowMsg("WiFi 已连接", WiFi.localIP().toString().c_str());

    // mDNS: http://esp32-clock.local
    if (MDNS.begin(HOSTNAME)) MDNS.addService("http", "tcp", 80);

    // NTP 对时
    configTzTime(TZ_INFO, NTP_SERVER1, NTP_SERVER2);

    // 天气后台任务（先创建互斥锁，再恢复城市设置）
    weatherStart();
    weatherSetCity(c);

    // 巴法云 MQTT（米家/小爱同学）
    bemfaStart();

    // HTTP 路由
    server.on("/", HTTP_GET, []() { server.send_P(200, "text/html", INDEX_HTML); });
    server.on("/api/data", HTTP_GET, handleApiData);
    server.on("/api/setcity", HTTP_GET, handleSetCity);
    server.onNotFound([]() { server.send(404, "application/json", "{\"error\":\"not found\"}"); });
    server.begin();

    Serial.printf("[Web] 时钟页面: http://%s 或 http://%s\n",
                  WiFi.localIP().toString().c_str(), HOSTNAME ".local");
}

void loop() {
    server.handleClient();
    unsigned long ms = millis();
    if (ms - lastSensorMs >= SENSOR_UPDATE_MS) {
        lastSensorMs = ms;
        readSensors();
    }
    displayLoop();
    ledLoop();
    delay(2);
}
