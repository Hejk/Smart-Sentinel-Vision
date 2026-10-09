#include "esp_camera.h"
#include "cam_module.h"
#include "config.h"

static bool s_inited = false;

bool cam_init(void)
{
    camera_config_t cfg = {};
    cfg.ledc_channel = LEDC_CHANNEL_0;      // XCLK 由 LEDC 外设产生
    cfg.ledc_timer   = LEDC_TIMER_0;
    cfg.pin_sccb_scl = CAM_PIN_SCCB_SCL;    // GPIO1 ← SIOC
    cfg.pin_sccb_sda = CAM_PIN_SCCB_SDA;    // GPIO2 ← SIOD
    cfg.pin_xclk     = CAM_PIN_XCLK;        // GPIO3
    cfg.pin_vsync    = CAM_PIN_VSYNC;       // GPIO4
    cfg.pin_href     = CAM_PIN_HREF;        // GPIO5
    cfg.pin_pclk     = CAM_PIN_PCLK;        // GPIO6
    cfg.pin_d0 = CAM_PIN_D0;  cfg.pin_d1 = CAM_PIN_D1;
    cfg.pin_d2 = CAM_PIN_D2;  cfg.pin_d3 = CAM_PIN_D3;
    cfg.pin_d4 = CAM_PIN_D4;  cfg.pin_d5 = CAM_PIN_D5;
    cfg.pin_d6 = CAM_PIN_D6;  cfg.pin_d7 = CAM_PIN_D7;
    cfg.pin_pwdn     = CAM_PIN_PWDN;        // -1 未接
    cfg.pin_reset    = CAM_PIN_RESET;       // GPIO15
    cfg.xclk_freq_hz = CAM_XCLK_HZ;
    cfg.pixel_format = PIXFORMAT_GRAYSCALE; // 灰度直出，直接喂 EI
    cfg.frame_size   = CAM_FRAME_SIZE;      // QVGA 320x240
    cfg.jpeg_quality = 12;                  // 仅 JPEG 格式时有效
    cfg.fb_count     = CAM_FB_COUNT;
    cfg.fb_location  = CAMERA_FB_IN_PSRAM;  // N16R8：8MB OPI PSRAM
    cfg.grab_mode    = CAMERA_GRAB_LATEST;  // 永远取最新帧，不积压旧帧

    esp_err_t err = esp_camera_init(&cfg);
    if (err != ESP_OK) {
        Serial.printf("[CAM] 初始化失败 0x%x（查 3V3/GND、XCLK、SCCB 两根线）\n", err);
        s_inited = false;
        return false;
    }

    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        // OV7670 的 PID = 0x76。若读到 0x00，多半 SIOC/SIOD 接反或虚接
        Serial.printf("[CAM] 传感器 PID=0x%x（OV7670 应为 0x76）\n", s->id.PID);
        // 成像上下/左右反了就改这两个 0/1 挨个试
        s->set_vflip(s, 0);
        s->set_hmirror(s, 0);
    }
    s_inited = true;
    Serial.printf("[CAM] OK：GRAYSCALE QVGA(320x240) fb=%d@PSRAM XCLK=%dMHz\n",
                  CAM_FB_COUNT, (int)(CAM_XCLK_HZ / 1000000));
    return true;
}

camera_fb_t* cam_capture(void)
{
    if (!s_inited) return NULL;
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb)
        Serial.println("[CAM] esp_camera_fb_get 失败（查 XCLK/PCLK/VSYNC/HREF 接线）");
    return fb;
}

void cam_release(camera_fb_t* fb)
{
    if (fb) esp_camera_fb_return(fb);
}

bool cam_scale_gray96(const camera_fb_t* fb, uint8_t* dst, size_t dst_len)
{
    if (!fb || !dst || dst_len < (size_t)(AI_IMG_W * AI_IMG_H)) return false;

    // 灰度帧每像素 1 字节：len == w*h。
    // 若 len 约 2 倍，说明抓到 RGB565，转换逻辑要另写（查 pixel_format 配置）
    if (fb->len < (size_t)fb->width * fb->height) {
        Serial.printf("[CAM] 帧不是灰度(len=%u w=%d h=%d)，缩放放弃\n",
                      (unsigned)fb->len, fb->width, fb->height);
        return false;
    }

    const int sw = fb->width, sh = fb->height;
    const uint8_t* src = fb->buf;

    // 块均值缩放：输出 1 像素 = 输入一块(约 3.3x2.5)的平均值
    for (int y = 0; y < AI_IMG_H; y++) {
        int y0 = y * sh / AI_IMG_H;
        int y1 = (y + 1) * sh / AI_IMG_H;
        if (y1 <= y0) y1 = y0 + 1;
        for (int x = 0; x < AI_IMG_W; x++) {
            int x0 = x * sw / AI_IMG_W;
            int x1 = (x + 1) * sw / AI_IMG_W;
            if (x1 <= x0) x1 = x0 + 1;
            uint32_t sum = 0, n = 0;
            for (int yy = y0; yy < y1; yy++) {
                const uint8_t* row = src + (size_t)yy * sw;
                for (int xx = x0; xx < x1; xx++) { sum += row[xx]; n++; }
            }
            dst[y * AI_IMG_W + x] = (uint8_t)(sum / n);
        }
    }
    return true;
}
