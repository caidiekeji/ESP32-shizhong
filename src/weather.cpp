#include "weather.h"
#include "config.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ArduinoUZlib.h>

WeatherData g_wx;
SemaphoreHandle_t wxMutex = nullptr;

static char s_locId[16] = "";              // 和风 LocationID，或 "lng,lat" 坐标
static volatile bool s_cityChanged = true;
static bool s_autoLoc = true;              // true=IP 自动定位
static char g_city[32] = "自动定位";        // 页面显示的城市名

// ---------- UTF-8 URL 编码 ----------
String urlEncode(const String &s) {
    String out;
    out.reserve(s.length() * 6);
    char buf[5];
    for (size_t i = 0; i < s.length(); ++i) {
        unsigned char c = (unsigned char)s[i];
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out += (char)c;
        } else {
            snprintf(buf, sizeof(buf), "%%%02X", c);
            out += buf;
        }
    }
    return out;
}

// ---------- 城市存取（加锁） ----------
String weatherGetCity() {
    if (xSemaphoreTake(wxMutex, pdMS_TO_TICKS(500))) {
        String c = g_city;
        xSemaphoreGive(wxMutex);
        return c;
    }
    return g_city;
}

bool weatherIsAuto() { return s_autoLoc; }

void weatherSetCity(const String &city) {
    if (xSemaphoreTake(wxMutex, pdMS_TO_TICKS(2000))) {
        if (city == "auto" || city.length() == 0) {
            s_autoLoc = true;
            strlcpy(g_city, "自动定位", sizeof(g_city));
        } else {
            s_autoLoc = false;
            strlcpy(g_city, city.c_str(), sizeof(g_city));
        }
        s_locId[0] = 0;          // 强制重新定位
        s_cityChanged = true;
        xSemaphoreGive(wxMutex);
    }
}

// ---------- HTTPS GET + JSON 解析 ----------
static bool httpGetJson(const char *host, const String &path, JsonDocument &doc) {
    WiFiClientSecure client;
    client.setInsecure();   // 演示项目跳过证书校验；生产环境建议绑定根证书

    HTTPClient http;
    String url = String("https://") + host + path;
    if (!http.begin(client, url)) return false;

    http.addHeader("Accept-Encoding", "identity");  // 能不压缩就不压缩
    http.setTimeout(12000);
    int code = http.GET();

    bool ok = false;
    if (code == 200) {
        const char *enc = http.header("Content-Encoding");
        if (enc && strstr(enc, "gzip")) {
            // 和风天气强制 gzip 响应：读原始字节 -> uzlib 解压 -> 再解析 JSON
            int len = http.getSize();
            uint8_t *in = nullptr;
            int got = 0;
            if (len > 0) {
                in = (uint8_t *)malloc(len);
                if (in) got = http.readBytes(in, len);
            } else {
                // chunked 分块：循环读完
                int cap = 2048;
                in = (uint8_t *)malloc(cap);
                WiFiClient *s = http.getStreamPtr();
                unsigned long lastData = millis();
                while (in && s && s->connected()) {
                    if (s->available()) {
                        if (got >= cap - 256) {
                            cap *= 2;
                            uint8_t *nb = (uint8_t *)realloc(in, cap);
                            if (!nb) { free(in); in = nullptr; break; }
                            in = nb;
                        }
                        got += s->readBytes(in + got, s->available());
                        lastData = millis();
                    } else if (millis() - lastData > 3000) {
                        break;
                    } else {
                        delay(2);
                    }
                }
                len = got;
            }
            if (in && got == len && len > 4) {
                uint8_t *out = nullptr;
                uint32_t outLen = 0;
                if (ArduinoUZlib::decompress(in, len, out, outLen) > 0 && out) {
                    ok = (deserializeJson(doc, out, outLen) == DeserializationError::Ok);
                    free(out);
                }
            }
            free(in);
        } else {
            String body = http.getString();
            ok = (deserializeJson(doc, body) == DeserializationError::Ok);
        }
    }
    http.end();
    return ok;
}

// ---------- IP 自动定位（xxapi） ----------
static bool fetchIPLocation() {
    JsonDocument doc;
    if (!httpGetJson("v2.xxapi.cn", "/api/ip", doc)) return false;
    if (doc["code"].as<int>() != 200) return false;

    const char *lat = doc["data"]["lat"] | "";
    const char *lng = doc["data"]["lng"] | "";
    const char *addr = doc["data"]["address"] | "";
    if (!lat[0] || !lng[0]) return false;

    // 经纬度直接作为和风天气 location 参数（格式：lng,lat），省去 GeoAPI
    String loc = String(lng) + "," + String(lat);
    strlcpy(s_locId, loc.c_str(), sizeof(s_locId));

    // 显示名：去掉 "中国" 前缀，如 "中国浙江省金华市永康市" -> "浙江省金华市永康市"
    String disp = addr;
    if (disp.startsWith("中国")) disp.remove(0, 6);
    if (disp.length() && disp != "自动定位") {
        if (xSemaphoreTake(wxMutex, pdMS_TO_TICKS(2000))) {
            strlcpy(g_city, disp.c_str(), sizeof(g_city));
            xSemaphoreGive(wxMutex);
        }
    }
    Serial.printf("[天气] IP定位成功: %s (%s)\n", disp.c_str(), s_locId);
    return true;
}

// ---------- 城市名 -> LocationID（和风 GeoAPI，手动城市时用） ----------
static bool fetchGeoLocation(const char *city) {
    JsonDocument doc;
    String path = String("/v2/city/lookup?location=") + urlEncode(city) +
                  "&key=" + QWEATHER_KEY + "&number=1&lang=zh";
    if (!httpGetJson(QWEATHER_GEO_HOST, path, doc)) return false;
    if (doc["code"].as<int>() != 200) return false;

    const char *id = doc["location"][0]["id"] | "";
    if (!id[0]) return false;
    strlcpy(s_locId, id, sizeof(s_locId));
    Serial.printf("[天气] 城市定位成功: %s -> %s\n", city, s_locId);
    return true;
}

static bool fetchLocation() {
    if (s_autoLoc) {
        if (fetchIPLocation()) return true;
        // IP 定位失败时退回默认城市
        Serial.println("[天气] IP定位失败，回退默认城市");
        return fetchGeoLocation(DEFAULT_CITY);
    }
    if (xSemaphoreTake(wxMutex, pdMS_TO_TICKS(2000))) {
        bool ok = fetchGeoLocation(g_city);
        xSemaphoreGive(wxMutex);
        return ok;
    }
    return false;
}

// ---------- 刷新一次天气 ----------
static void refreshWeather() {
    JsonDocument docNow, docDaily;

    String p1 = String("/v7/weather/now?location=") + s_locId + "&key=" + QWEATHER_KEY + "&lang=zh";
    String p2 = String("/v7/weather/3d?location=") + s_locId + "&key=" + QWEATHER_KEY + "&lang=zh";

    bool okN = httpGetJson(QWEATHER_API_HOST, p1, docNow) && docNow["code"].as<int>() == 200;
    bool okD = httpGetJson(QWEATHER_API_HOST, p2, docDaily) && docDaily["code"].as<int>() == 200;
    if (!okN && !okD) {
        Serial.println("[天气] 拉取失败");
        return;
    }

    if (xSemaphoreTake(wxMutex, pdMS_TO_TICKS(2000))) {
        if (okN) {
            JsonObject n = docNow["now"];
            strlcpy(g_wx.text,    (const char*)(n["text"]    | "--"), sizeof(g_wx.text));
            strlcpy(g_wx.windDir, (const char*)(n["windDir"] | ""),   sizeof(g_wx.windDir));
            g_wx.icon      = n["icon"]      | 0;
            g_wx.temp      = n["temp"]      | 0.0f;
            g_wx.feels     = n["feelsLike"] | 0.0f;
            g_wx.hum       = n["humidity"]  | 0.0f;
            g_wx.pressure  = n["pressure"]  | 0.0f;
            g_wx.windScale = n["windScale"] | 0;
            g_wx.updated   = time(nullptr);
            g_wx.valid     = true;
        }
        if (okD) {
            int i = 0;
            for (JsonObject d : docDaily["daily"].as<JsonArray>()) {
                if (i >= 3) break;
                strlcpy(g_wx.fc[i].date, (const char*)(d["fxDate"]  | ""),   sizeof(g_wx.fc[i].date));
                strlcpy(g_wx.fc[i].text, (const char*)(d["textDay"] | "--"), sizeof(g_wx.fc[i].text));
                g_wx.fc[i].icon = d["iconDay"] | 0;
                g_wx.fc[i].tmax = d["tempMax"] | 0;
                g_wx.fc[i].tmin = d["tempMin"] | 0;
                i++;
            }
            g_wx.fcCount = i;
        }
        xSemaphoreGive(wxMutex);
        Serial.println("[天气] 更新完成");
    }
}

// ---------- 天气任务 ----------
static void weatherTask(void *) {
    vTaskDelay(pdMS_TO_TICKS(4000));   // 等WiFi/NTP稳定
    if (strlen(QWEATHER_KEY) == 0) {
        Serial.println("[天气] 未配置 QWEATHER_KEY！本地编译请先 set QWEATHER_KEY=你的key，"
                       "GitHub 编译请在仓库 Secrets 中添加 QWEATHER_KEY");
        vTaskDelete(nullptr);
        return;
    }
    for (;;) {
        TickType_t wait = pdMS_TO_TICKS(WEATHER_UPDATE_MINUTES * 60UL * 1000UL);
        if (WiFi.status() == WL_CONNECTED) {
            if (s_cityChanged || s_locId[0] == 0) {
                s_cityChanged = false;
                if (fetchLocation()) {
                    refreshWeather();
                } else {
                    Serial.println("[天气] 定位失败，60s后重试");
                    wait = pdMS_TO_TICKS(60000);
                }
            } else {
                // 自动定位模式下随天气一起重新定位（如出差/移动场景）
                if (s_autoLoc && !fetchIPLocation()) {
                    Serial.println("[天气] IP定位失败，沿用上次位置");
                }
                refreshWeather();
            }
        } else {
            wait = pdMS_TO_TICKS(10000);
        }
        vTaskDelay(wait);
    }
}

void weatherStart() {
    wxMutex = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(weatherTask, "weather", 12288, nullptr, 1, nullptr, 0);
}
