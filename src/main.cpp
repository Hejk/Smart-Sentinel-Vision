// ============================================================
//  智哨 Smart Sentinel · AI 视觉主控 v0.1（占位推理版）
//  ESP32-S3-N16R8 + OV7670 → 96x96 灰度 → AI推理(占位) → UART1 JSON
//
//  两个串口：
//   · USB 口（板上标 USB）：日志 + 调试命令
//   · UART1（TX17/RX18）  ：与主控板 JSON 通信
//
//  USB 调试命令：
//   c = 采图+推理+上报    p = 打印 96x96 灰度 ASCII 预览
//   i = 运行信息          r = 重试摄像头初始化
//   0~5 = 强制占位推理输出指定类别
// ============================================================
#include <Arduino.h>
#include "config.h"
#include "cam_module.h"
#include "ai_infer.h"
#include "ai_link.h"

// 96x96 = 9.2KB，必须放全局：loopTask 栈只有 8KB，栈上开会溢出
static uint8_t  s_gray96[AI_IMG_W * AI_IMG_H];
static bool     s_has_frame = false;    // 有有效的缩放结果可供预览
static uint32_t s_frame_cnt = 0;
static uint32_t s_last_auto = 0;
static bool     s_cam_ok    = false;

// 一轮完整动作：采集 → 缩放 → 推理 → UART 上报
static void do_capture_infer_send(const char* trigger)
{
    camera_fb_t* fb = cam_capture();
    if (!fb) { Serial.printf("[MAIN] 采集失败(%s)\n", trigger); return; }
    s_frame_cnt++;

    bool ok = cam_scale_gray96(fb, s_gray96, sizeof(s_gray96));
    cam_release(fb);                    // 立刻还帧，别占着双缓冲
    if (!ok) return;
    s_has_frame = true;

    AiResult r;
    if (ai_infer_gray96(s_gray96, sizeof(s_gray96), &r)) {
        Serial.printf("[MAIN] 触发=%s 推理=%s(%s) conf=%.2f\n",
                      trigger, r.animal_en, r.label_cn, r.confidence);
        ai_link_send_result(r);         // → UART1: {"type":"result",...}
    }
}

static void print_info(void)
{
    char forcedTxt[32];
    if (ai_infer_forced_active())
        snprintf(forcedTxt, sizeof(forcedTxt), "%s/%s",
                 ANIMAL_EN[ai_infer_get_forced()], ANIMAL_CN[ai_infer_get_forced()]);
    else
        snprintf(forcedTxt, sizeof(forcedTxt), "off(EI真模型)");

    Serial.printf("[INFO] heap=%uKB psram=%uKB frames=%u cam=%d "
                  "forced=%s auto=%ds\n",
                  (unsigned)(ESP.getFreeHeap() / 1024),
                  (unsigned)(ESP.getFreePsram() / 1024),
                  (unsigned)s_frame_cnt, (int)s_cam_ok,
                  forcedTxt, AUTO_CAPTURE_SEC);
}

static void print_preview(void)
{
    if (!s_has_frame) { Serial.println("[MAIN] 还没有帧，先发 c 采一张"); return; }
    Serial.println("[PREVIEW] 96x96 灰度 → 48x24 ASCII（越亮越接近 @）：");
    static const char* ramp = " .:-=+*#%@";
    for (int y = 0; y < 24; y++) {
        char line[50];
        int li = 0;
        for (int x = 0; x < 48; x++) {
            uint32_t sum = 0;
            for (int dy = 0; dy < 4; dy++)          // 2x4 像素聚一个字符
                for (int dx = 0; dx < 2; dx++)
                    sum += s_gray96[(y * 4 + dy) * AI_IMG_W + (x * 2 + dx)];
            int v = (int)(sum / 8) * 9 / 255;
            if (v > 9) v = 9;
            line[li++] = ramp[v];
        }
        line[li] = '\0';
        Serial.println(line);
    }
}

static void handle_usb_cmd(char c)
{
    switch (c) {
    case 'c': do_capture_infer_send("usb"); return;
    case 'p': print_preview();  return;
    case 'i': print_info();     return;
    case 'r':
        Serial.println("[USB] 重试摄像头初始化…");
        s_cam_ok = cam_init();
        return;
    default:
        if (c >= '0' && c <= '0' + AN_COUNT - 1) {
            ai_infer_set_forced((uint8_t)(c - '0'));
            Serial.printf("[USB] 强制类别=%s(%s)，发 o 恢复真模型\n",
                          ANIMAL_EN[c - '0'], ANIMAL_CN[c - '0']);
            return;
        }
        if (c == 'o') {
            ai_infer_clear_forced();
            Serial.println("[USB] 已恢复真模型推理");
            return;
        }
        if (c != '\r' && c != '\n')
            Serial.println("[USB] 命令：c=采图推理 p=预览 i=信息 r=重试摄像头 0~5=强制类别 o=恢复真模型");
    }
}

void setup()
{
    Serial.begin(115200);
    uint32_t t0 = millis();
    while (!Serial && millis() - t0 < 3000) delay(10);  // 等 USB CDC 挂上

    Serial.println("\n==============================================");
    Serial.println("  智哨 Smart Sentinel · AI 视觉主控 v0.1");
    Serial.println("  OV7670 → 96x96 灰度 → AI推理(占位) → UART1");
    Serial.println("==============================================");
    Serial.printf("[HW] heap=%uKB psram=%uKB（N16R8 应见 8192KB PSRAM）\n",
                  (unsigned)(ESP.getFreeHeap() / 1024),
                  (unsigned)(ESP.getFreePsram() / 1024));
    Serial.println("[HW] OV7670: SCL=1 SDA=2 XCLK=3 VSYNC=4 HREF=5 PCLK=6 D0~D7=7~14 RST=15");
    Serial.printf("[HW] UART1:  TX=17 RX=18 @%d（与主控交叉相接）\n", AI_UART_BAUD);
    Serial.println("[USB] 命令：c=采图推理 p=预览 i=信息 r=重试摄像头 0~5=强制类别 o=恢复真模型");

    if (!psramFound())
        Serial.println("[HW] ★警告：PSRAM 未启用！检查 memory_type=qio_opi 配置");

    ai_link_begin();
    s_cam_ok = cam_init();
    if (!s_cam_ok)
        Serial.println("[MAIN] 摄像头初始化失败：查 DVP 18 线接线，然后按 r 重试");
}

void loop()
{
    // 1) 主控 UART 命令 {"cmd":"capture"} → 一轮完整动作
    if (ai_link_poll())
        do_capture_infer_send("uart");

    // 2) USB 调试命令
    while (Serial.available())
        handle_usb_cmd((char)Serial.read());

    // 3) 台架自测：无触发也周期上报，单板就能验证"采集→推理→协议"整条链路
#if AUTO_CAPTURE_SEC > 0
    if (s_cam_ok && millis() - s_last_auto >= (uint32_t)AUTO_CAPTURE_SEC * 1000UL) {
        s_last_auto = millis();
        do_capture_infer_send("auto");
    }
#endif

    delay(2);
}
