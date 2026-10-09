# Smart-Sentinel-Vision · 智哨 AI 视觉主控

智哨 Smart Sentinel（基于端云协同的农田生态防御系统）的**视觉识别节点**固件。
与主控节点固件仓库 [Smart-Sentinel](https://github.com/Hejk/Smart-Sentinel) 通过 UART 一行式 JSON 协议互联，代码不共享、只对齐协议。

## 硬件

| 部件 | 型号 / 接口 |
|---|---|
| 主控 | ESP32-S3-N16R8（16MB QIO Flash + 8MB OPI PSRAM） |
| 摄像头 | OV7670，DVP 18 线 |
| 对外通信 | UART1（与主控交叉相接） |

## 引脚映射（与 config.h 一一对应，改线必同步改码）

```
SCL/SIOC=1  SDA/SIOD=2  XCLK=3  VSYNC=4  HREF=5  PCLK=6
D0~D7 = 7~14        RESET=15        PWDN 未接
UART1: TX=17  RX=18  @115200
```

## 通信协议（\n 结尾，一行一条）

```json
AI → 主控   {"type":"result","animal":"wild_boar","confidence":0.85,"label_cn":"野猪"}
主控 → AI   {"cmd":"capture"}
链路测试    {"cmd":"ping"} → {"type":"pong","from":"ai"}
```

识别 6 类：`wild_boar` 野猪 / `roe_deer` 狍子 / `squirrel` 松鼠 / `badger` 獾子 / `bird` 飞鸟 / `human` 人类

## 数据链路

OV7670 → esp32-camera 抓 QVGA 320×240 灰度帧（PSRAM 双缓冲）→ 块均值缩放 96×96 → AI 推理 → UART1 上报

## 编译烧录

PlatformIO（Arduino 框架）：

```bash
pio run                     # 编译
pio run -t upload           # 烧录（板上标 USB 的口）
pio device monitor          # 日志 + 调试命令 @115200
```

USB 串口命令：`c`=采图推理 `p`=ASCII预览 `i`=信息 `r`=重试摄像头 `0~5`=强制占位类别

## AI 模型

已接入 Edge Impulse 真模型（3类：bird/unknown/wildboar，96x96，<0.4 归 unknown）。zip 在 `edge-impulse/`，解压为 `edge-impulse/model-v2/`（gitignore，不入库）。
替换 Edge Impulse 真模型的 5 步操作写在 `ai_infer.cpp` 顶部注释里，对外接口不变。

## 版本

- `dev/v2` — 开发主线（v0.1 占位推理版）
