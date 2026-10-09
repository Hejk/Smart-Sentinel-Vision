#pragma once
#include <Arduino.h>
#include "ai_infer.h"

// 初始化 UART1：TX=17 发结果，RX=18 收主控命令，115200 8N1
bool ai_link_begin(void);

// 轮询 RX 行缓冲，解析 {"cmd":"capture"}。
// 收到 capture 返回 true（main 负责执行 采集→推理→上报）
bool ai_link_poll(void);

// 上报识别结果（\n 结尾，不带 \r）：
//   {"type":"result","animal":"wild_boar","confidence":0.85,"label_cn":"野猪"}
bool ai_link_send_result(const AiResult& r);
