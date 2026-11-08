#pragma once
#include <Arduino.h>

// 巴法云 MQTT 模块：
//  - 每 5 分钟向传感器主题上报 "#温度#湿度#"，米家绑定后可查询、可联动
//  - 若配置了开关主题，可被小爱同学语音控制 OLED 屏开/关
void bemfaStart();
