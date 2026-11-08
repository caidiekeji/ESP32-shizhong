#include "led.h"
#include "config.h"
#include <WiFi.h>

// 兼容 Arduino-ESP32 2.x / 3.x 的板载 RGB 写入接口
static inline void rgbWrite(uint8_t r, uint8_t g, uint8_t b) {
#if defined(ESP_ARDUINO_VERSION) && ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    rgbLedWrite(LED_PIN, r, g, b);
#else
    neopixelWrite(LED_PIN, r, g, b);
#endif
}

static unsigned long s_greenMs = 0;
static bool           s_done   = false;   // 已完成"绿灯->熄灭"流程

void ledInit() {
    rgbWrite(LED_BRIGHTNESS, 0, 0);    // 红灯：启动中
}

void ledConnected() {
    rgbWrite(0, LED_BRIGHTNESS, 0);    // 绿灯：联网成功
    s_greenMs = millis();
    s_done = false;
}

void ledLoop() {
    if (s_done) {
        // 熄灯后如果 WiFi 掉线，重新点亮红灯提示
        if (WiFi.status() != WL_CONNECTED) {
            static unsigned long s_blink = 0;
            if (millis() - s_blink > 500) {
                s_blink = millis();
                static bool on = false;
                on = !on;
                if (on) rgbWrite(LED_BRIGHTNESS, 0, 0);
                else    rgbWrite(0, 0, 0);
            }
        }
        return;
    }

    if (s_greenMs == 0) {
        // 还未联网成功：慢闪红灯（配网/连接中）
        static unsigned long s_blink = 0;
        if (millis() - s_blink > (WiFi.status() == WL_CONNECTED ? 100000 : 800)) {
            s_blink = millis();
            static bool on = true;
            on = !on;
            if (on) rgbWrite(LED_BRIGHTNESS, 0, 0);
            else    rgbWrite(0, 0, 0);
        }
        return;
    }

    // 绿灯保持 LED_ON_MS 后熄灭
    if (millis() - s_greenMs >= LED_ON_MS) {
        rgbWrite(0, 0, 0);
        s_done = true;
    }
}
