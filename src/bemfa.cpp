#include "bemfa.h"
#include "config.h"
#include "state.h"
#include "display.h"
#include <WiFi.h>
#include <PubSubClient.h>

static WiFiClient s_net;
static PubSubClient s_mqtt(s_net);

static void onMessage(char *topic, byte *payload, unsigned int len) {
    if (!strlen(BEMFA_TOPIC_SWITCH)) return;
    // 仅处理开关主题
    if (strncmp(topic, BEMFA_TOPIC_SWITCH, strlen(BEMFA_TOPIC_SWITCH)) != 0) return;

    char cmd[8] = {0};
    for (unsigned int i = 0; i < len && i < sizeof(cmd) - 1; i++)
        cmd[i] = (char)tolower(payload[i]);

    bool on = (strcmp(cmd, "on") == 0);
    displaySetEnabled(on);

    // 用 /up 只更新云端状态，不触发推送，避免自环
    char up[96];
    snprintf(up, sizeof(up), "%s/up", BEMFA_TOPIC_SWITCH);
    s_mqtt.publish(up, on ? "on" : "off");
    Serial.printf("[巴法云] 收到控制指令: %s\n", on ? "on" : "off");
}

static bool connectMQTT() {
    s_mqtt.setServer(BEMFA_HOST, BEMFA_PORT);
    s_mqtt.setCallback(onMessage);
    s_mqtt.setKeepAlive(30);     // 巴法云要求心跳 ≤ 60s
    s_mqtt.setSocketTimeout(5);
    // 巴法云：私钥作为 clientId，无需用户名密码
    if (!s_mqtt.connect(BEMFA_UID)) return false;

    if (strlen(BEMFA_TOPIC_SWITCH)) {
        s_mqtt.subscribe(BEMFA_TOPIC_SWITCH);
    }
    return true;
}

static void bemfaTask(void *) {
    if (strlen(BEMFA_UID) == 0) {
        Serial.println("[巴法云] 未配置 BEMFA_UID，米家接入跳过（config.h 中设置或环境变量注入）");
        vTaskDelete(nullptr);
        return;
    }
    vTaskDelay(pdMS_TO_TICKS(6000));   // 等WiFi稳定

    unsigned long lastPub = 0;
    for (;;) {
        if (WiFi.status() != WL_CONNECTED) {
            vTaskDelay(pdMS_TO_TICKS(10000));
            continue;
        }
        if (!s_mqtt.connected()) {
            if (!connectMQTT()) {
                Serial.printf("[巴法云] 连接失败 state=%d，30s后重试\n", s_mqtt.state());
                vTaskDelay(pdMS_TO_TICKS(30000));
                continue;
            }
            Serial.println("[巴法云] MQTT 已连接");
        }
        s_mqtt.loop();   // 心跳 + 收消息

        // 定时上报温湿度：#温度#湿度#
        unsigned long interval = BEMFA_REPORT_MINUTES * 60UL * 1000UL;
        if ((lastPub == 0 || millis() - lastPub >= interval) &&
            !isnan(indoor.t) && !isnan(indoor.h)) {
            char msg[32];
            snprintf(msg, sizeof(msg), "#%.1f#%.1f#", indoor.t, indoor.h);
            if (s_mqtt.publish(BEMFA_TOPIC_SENSOR, msg)) {
                lastPub = millis();
                Serial.printf("[巴法云] 上报 %s\n", msg);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void bemfaStart() {
    xTaskCreatePinnedToCore(bemfaTask, "bemfa", 8192, nullptr, 1, nullptr, 0);
}
