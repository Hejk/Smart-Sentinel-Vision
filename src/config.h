#pragma once
#include <Arduino.h>

// ============================================================
//  智哨 Smart Sentinel · AI 视觉主控（ESP32-S3_1 / N16R8）
//  职责：OV7670 采图 → 缩放 96x96 灰度 → AI 推理(占位) → UART1 JSON 上报
//  对端：主控 ESP32-S3（code-project-1，dev/v2 分支）
//
//  通信协议（\n 结尾，一行一条）：
//    AI主控→主控  {"type":"result","animal":"wild_boar","confidence":0.85,"label_cn":"野猪"}
//    主控→AI主控  {"cmd":"capture"}
// ============================================================

// —— OV7670 DVP 接口（模块 18 线：16 信号 + 3V3 + GND）
// ★ 与工作台接线一一对应，改线必须同步改这里
#define CAM_PIN_SCCB_SCL   1     // OV7670 SIOC（时钟线，模块上标 SCL/SIOC）
#define CAM_PIN_SCCB_SDA   2     // OV7670 SIOD（数据线，模块上标 SDA/SIOD）
#define CAM_PIN_XCLK       3
#define CAM_PIN_VSYNC      4
#define CAM_PIN_HREF       5
#define CAM_PIN_PCLK       6
#define CAM_PIN_D0         7
#define CAM_PIN_D1         8
#define CAM_PIN_D2         9
#define CAM_PIN_D3         10
#define CAM_PIN_D4         11
#define CAM_PIN_D5         12
#define CAM_PIN_D6         13
#define CAM_PIN_D7         14
#define CAM_PIN_RESET      15
#define CAM_PIN_PWDN       -1    // 未接（OV7670 模块自带上电复位）

// OV7670 无 JPEG 引擎，只能输出 RGB565 / YUV / 灰度原始帧。
// Edge Impulse 图像分类常用 96x96 灰度输入，这里直接抓灰度：
// QVGA 灰度帧仅 76.8KB，带宽减半，缩放时也省一步颜色转换。
#define CAM_XCLK_HZ        20000000UL  // 花屏/采不到帧就降到 10000000（不少兼容模块只能跑 10M）
#define CAM_FB_COUNT       2           // 双缓冲，放 PSRAM
#define CAM_FRAME_SIZE     FRAMESIZE_QVGA  // 320x240 → 缩放 96x96 用均值采样

// —— AI 推理输入规格（对应 Edge Impulse 工程的图像尺寸 96x96x1）
#define AI_IMG_W           96
#define AI_IMG_H           96

// —— UART1 互联（两板交叉相接：本板TX17→主控RX18，主控TX17→本板RX18）
#define AI_UART_PORT       1
#define AI_UART_TX         17
#define AI_UART_RX         18
#define AI_UART_BAUD       115200

// —— 台架自测：没有主控触发时每 N 秒自动采图+推理+上报一次，0=只被动响应主控
#define AUTO_CAPTURE_SEC   10
