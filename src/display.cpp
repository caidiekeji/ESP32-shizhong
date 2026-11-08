#include "display.h"
#include "config.h"
#include "state.h"
#include "weather.h"
#include <U8g2lib.h>
#include <time.h>

// 128x32 SSD1306，硬件 I2C（setup 中已 Wire.begin(SDA,SCL)，S3 默认即 8/9）
static U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
static bool s_ok = false;
static unsigned long s_lastDraw = 0, s_lastPage = 0;
static uint8_t s_page = 0;

static const char WDAY[] = "日一二三四五六";   // UTF-8 每字 3 字节

static bool busProbe(uint8_t addr) {
    Wire.beginTransmission(addr);
    return Wire.endTransmission() == 0;
}

bool displayInit() {
    if (!busProbe(0x3C) && !busProbe(0x3D)) {
        Serial.println("[屏幕] 未检测到 SSD1306 (0x3C/0x3D)，不启用屏幕");
        return false;
    }
    s_ok = u8g2.begin();
    if (s_ok) Serial.println("[屏幕] SSD1306 128x32 初始化成功");
    return s_ok;
}

bool displayOK() { return s_ok; }

void displayShowMsg(const char *l1, const char *l2) {
    if (!s_ok) return;
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_wqy12_t_gb2312);
    u8g2.drawUTF8(0, 12, l1);
    u8g2.drawUTF8(0, 28, l2);
    u8g2.sendBuffer();
}

// ---------- 每秒刷新 ----------
static void draw() {
    struct tm t;
    time_t now = time(nullptr);
    localtime_r(&now, &t);
    bool timeOK = (t.tm_year + 1900) >= 2024;   // NTP 未同步时显示占位

    u8g2.clearBuffer();

    if (s_page == 0) {
        // ---- 第 1 页：大时钟 + 日期 ----
        char buf[8];
        if (timeOK) snprintf(buf, sizeof(buf), "%02d:%02d", t.tm_hour, t.tm_min);
        else        strcpy(buf, "--:--");

        u8g2.setFont(u8g2_font_logisoso22_tn);
        u8g2.drawStr(0, 21, buf);
        int w = u8g2.getStrWidth(buf);

        if (timeOK) {
            char sec[6];
            snprintf(sec, sizeof(sec), ":%02d", t.tm_sec);
            u8g2.setFont(u8g2_font_logisoso16_tn);
            u8g2.drawStr(w + 3, 17, sec);
        }

        u8g2.setFont(u8g2_font_wqy12_t_gb2312);
        char d[28], wk[4];
        if (timeOK) {
            memcpy(wk, WDAY + t.tm_wday * 3, 3);
            wk[3] = 0;
            snprintf(d, sizeof(d), "%d月%d日 周%s", t.tm_mon + 1, t.tm_mday, wk);
        } else {
            strcpy(d, "等待网络对时…");
        }
        u8g2.drawUTF8(0, 32, d);
    } else {
        // ---- 第 2 页：天气 + 室内环境 ----
        u8g2.setFont(u8g2_font_wqy12_t_gb2312);

        char l1[56] = "天气获取中…";
        String city = weatherGetCity();
        if (xSemaphoreTake(wxMutex, pdMS_TO_TICKS(200))) {
            if (g_wx.valid) {
                snprintf(l1, sizeof(l1), "%s %s %g°C", city.c_str(), g_wx.text, g_wx.temp);
            }
            xSemaphoreGive(wxMutex);
        }
        u8g2.drawUTF8(0, 13, l1);

        char l2[48];
        if (isnan(indoor.t)) {
            strcpy(l2, "传感器未连接");
        } else if (isnan(indoor.p)) {
            snprintf(l2, sizeof(l2), "室内 %g°C %g%%", indoor.t, indoor.h);
        } else {
            snprintf(l2, sizeof(l2), "室内 %g°C %g%% %dhPa", indoor.t, indoor.h, (int)(indoor.p + 0.5f));
        }
        u8g2.drawUTF8(0, 29, l2);
    }
    u8g2.sendBuffer();
}

void displayLoop() {
    if (!s_ok) return;
    unsigned long ms = millis();
    if (ms - s_lastDraw < 1000) return;
    s_lastDraw = ms;
    if (ms - s_lastPage > 6000) {   // 每 6 秒翻页
        s_lastPage = ms;
        s_page ^= 1;
    }
    draw();
}
