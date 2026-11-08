# ESP32 智能时钟（网页版桌面时钟）

基于 **ESP32-S3 N16R8**（16MB Flash + 8MB PSRAM）的桌面时钟，浏览器打开即为漂亮的网页时钟，可选配 0.91 寸 OLED 小屏离线显示。

## 功能

- 🕐 **NTP 自动对时**（阿里云 NTP，UTC+8，掉电不丢，联网即准）
- 🖥 **网页时钟**：玻璃拟态深色 UI，白天/夜间配色自动切换
- 📺 **0.91" OLED 显示**：SSD1306 128×32，时钟页/天气页 6 秒交替
- 🌡 **室内环境**：AHT20 温湿度 + BMP280 气压，5 秒刷新
- ☁️ **和风天气**：实时天气 + 3 天预报，20 分钟自动刷新（HTTPS 拉取）
- 📍 **IP 自动定位**：xxapi 接口取经纬度直查天气，出差搬家天气跟着走
- 📶 **首次免代码配网**：开机自动创建热点 `ESP32-Clock-Setup`（密码 `12345678`），手机连上即可选 WiFi
- 🏙 **城市随意改**：页面右下角输入城市名即可切换，保存在 Flash 中；点「自动」恢复 IP 定位
- 💡 **RGB 状态灯**（板载 WS2812，GPIO48）：红=启动/连网中，绿=联网成功（1 分钟后自动熄灭），掉线红灯闪
- 🔗 访问方式：`http://<板子IP>` 或 `http://esp32-clock.local`

## 接线

| 模块引脚 | ESP32-S3 引脚 |
|---|---|
| AHT20 + BMP280 模块 SDA | GPIO 8 |
| SCL | GPIO 9 |
| VCC | 3.3V |
| GND | GND |
| 0.91" OLED (SSD1306) SDA | GPIO 8（与传感器并联） |
| 0.91" OLED SCL | GPIO 9（与传感器并联） |
| 0.91" OLED VCC / GND | 3.3V / GND |

> OLED 地址 0x3C，与传感器 0x38/0x76 不冲突，直接并到同一条 I2C 总线即可；不接屏幕也不影响运行（自动检测跳过）。
> BMP280 模块的 SDO 引脚接 GND（地址 0x76，默认）；若接 VCC 请把 `src/config.h` 中 `BMP280_ADDR` 改为 `0x77`。
> 经典 ESP32（非 S3）请用 GPIO 21/22，并修改 `platformio.ini`（见文件内注释）。

## 编译方式

### 方式一：GitHub Actions 云端编译（无需装任何环境）

1. Fork 或使用本仓库，进入 **Settings → Secrets and variables → Actions → New repository secret**
2. Name 填 `QWEATHER_KEY`，Secret 填你的和风天气 Key
3. 进入 **Actions → Build Firmware → Run workflow**，跑完后在本次运行页面底部下载 `firmware.bin` 制品
4. 用 [esptool](https://github.com/espressif/esptool) 烧录：
   `esptool.py --chip esp32s3 --port COM口 --baud 921600 write_flash 0x10000 firmware.bin`

### 方式二：本地 PlatformIO

Key 通过环境变量注入（不写入源码）：

```bash
# Windows (CMD，永久生效用 setx)
set QWEATHER_KEY=你的和风Key
pio run -t upload        # 编译并烧录
pio device monitor       # 查看串口日志（115200）
```

VS Code + PlatformIO 插件亦可：打开项目 → Upload。

## 首次使用

1. 烧录后上电，板子会创建热点 **ESP32-Clock-Setup**（密码 12345678）
2. 手机连接该热点，自动弹出配网页，选择你家 WiFi 并输入密码
3. 配网成功后，串口日志会打印板子的 IP
4. 浏览器打开 `http://<IP>` 或 `http://esp32-clock.local`，即可看到时钟

## 和风天气 API Host 说明

- **2024年9月之前注册的账号**：无需改动，代码里默认用 `devapi.qweather.com`
- **之后注册的账号**：登录和风天气控制台 → 应用管理，会看到一个**专属 API Host**（形如 `abc123.re.qweatherapi.com`），把 `src/config.h` 里的 `QWEATHER_API_HOST` 和 `QWEATHER_GEO_HOST` 都改成它
- 你的 Key 已预置在 `src/config.h` 的 `QWEATHER_KEY`，免费订阅每天有 1000 次额度，本项目每 20 分钟 2 次，绰绰有余

## HTTP API（页面在用，也可自己调用）

| 接口 | 说明 |
|---|---|
| `GET /` | 时钟页面 |
| `GET /api/data` | 全量数据：时间、室内温湿压、天气、预报、系统状态 |
| `GET /api/setcity?city=上海` | 修改城市（持久化保存，立即生效） |
| `GET /api/setcity?city=auto` | 开启 IP 自动定位（xxapi `v2.xxapi.cn/api/ip`，取经纬度直查天气） |

`/api/data` 返回示例：

```json
{
  "time": 1762611600, "city": "北京",
  "wx": {"valid": true, "text": "晴", "icon": 100, "temp": 12.5, "feels": 10.0,
          "hum": 40, "wind": "西北风 3级", "p": 1013, "updated": 1762610400},
  "fc": [{"date":"2026-11-09","text":"晴","icon":100,"max":15,"min":4}],
  "indoor": {"t": 23.5, "h": 45.2, "p": 1012.8, "aht": true, "bmp": true},
  "sys": {"heap": 190000, "rssi": -58, "up": 3600}
}
```

## 项目结构

```
ESP32-shizhong/
├── platformio.ini        # 板型/分区/依赖库配置（S3 N16R8）
└── src/
    ├── config.h          # ★ 所有可调参数：引脚、城市、天气Key、刷新周期
    ├── main.cpp          # 启动流程、WiFi配网、传感器采集、Web服务
    ├── weather.cpp/.h    # 和风天气后台任务（GeoAPI 定位 + 实时 + 3日预报）
    └── web_pages.h       # 内嵌网页时钟 UI（无需上传文件系统）
```

## 接入米家 / 小爱同学（巴法云）

1. 注册 [巴法云控制台](https://cloud.bemfa.com)，首页复制**私钥**
2. **MQTT设备云** 创建一个主题，**尾号必须是 004**（温湿度传感器类型）
3. 仓库 `Settings → Secrets → Actions` 添加 `BEMFA_UID`（填私钥）；本地编译则 `set BEMFA_UID=私钥`
4. 把主题名填到 `src/config.h` 的 `BEMFA_TOPIC_SENSOR`
5. **米家 App → 我的 → 其他平台设备 → 添加 → 搜索「巴法」→ 登录巴法云账号**，设备同步进米家

之后可以：对小爱说「**查询传感器温度 / 湿度**」；在米家做联动（如温度高于 28°C 自动开风扇）。气压类数据米家不支持，仅网页/OLED 显示。

> 可选：再创建一个尾号 001 的开关主题，填入 `BEMFA_TOPIC_SWITCH`，就能对小爱说「打开/关闭时钟屏幕」控制 OLED 亮灭。

## 常见问题

- **页面打开是 404/打不开**：确认浏览器和板子在同一局域网；`.local` 域名在部分安卓浏览器不可用，建议直接用 IP
- **天气一直"获取中"**：串口看日志；多为 API Host 不对（见上文说明）或免费 Key 未绑定该 Host
- **BMP280 读不到**：检查 SDO 接法与 `BMP280_ADDR`
- **改了配置如何生效**：重新编译烧录；城市修改例外，网页上改即时生效且掉电保存
