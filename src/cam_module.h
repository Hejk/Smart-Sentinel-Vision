#pragma once
#include <Arduino.h>
#include "esp_camera.h"

// 初始化 OV7670（引脚/XCLK/帧格式见 config.h），成功后打印传感器 PID
bool cam_init(void);

// 抓一帧。用完必须 cam_release() 还回去，否则双缓冲耗尽后永远抓不到新帧
camera_fb_t* cam_capture(void);
void         cam_release(camera_fb_t* fb);

// 把当前帧缩放到 96x96 灰度（块均值采样，等效抗锯齿）。
// 仅支持灰度帧（本项目像素格式就是 PIXFORMAT_GRAYSCALE）
bool cam_scale_gray96(const camera_fb_t* fb, uint8_t* dst, size_t dst_len);
