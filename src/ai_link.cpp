#include <ArduinoJson.h>
#include "ai_link.h"
#include "config.h"

static HardwareSerial s_uart(AI_UART_PORT);
static char   s_line[192];
static size_t s_len = 0;

bool ai_link_begin(void)
{
    s_uart.setRxBufferSize(1024);
    s_uart.begin(AI_UART_BAUD, SERIAL_8N1, AI_UART_RX, AI_UART_TX);
    return (bool)s_uart;
}

// 解析一行 JSON。返回 true 表示主控要求采集
static bool handle_line(const char* line)
{
    if (!line[0]) return false;                      // 空行忽略

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, line);
    if (err) {
        Serial.printf("[UART1<-] 非JSON(%s): %s\n", err.c_str(), line);
        return false;
    }

    const char* cmd = doc["cmd"] | "";
    if (strcmp(cmd, "capture") == 0) {
        Serial.println("[UART1<-] {\"cmd\":\"capture\"} → 采集+推理+上报");
        return true;
    }
    if (strcmp(cmd, "ping") == 0) {                  // 附加：链路连通性测试
        s_uart.print("{\"type\":\"pong\",\"from\":\"ai\"}\n");
        Serial.println("[UART1<-] ping → 已回 pong");
        return false;
    }
    Serial.printf("[UART1<-] 未知命令: %s\n", line);
    return false;
}

bool ai_link_poll(void)
{
    bool capture_req = false;
    while (s_uart.available()) {
        char ch = (char)s_uart.read();
        if (ch == '\n') {
            s_line[s_len] = '\0';
            if (handle_line(s_line)) capture_req = true;
            s_len = 0;
        } else if (ch != '\r') {                     // 容忍对端发 \r\n
            if (s_len < sizeof(s_line) - 1) {
                s_line[s_len++] = ch;
            } else {                                 // 超长坏行，丢弃重来
                s_len = 0;
                Serial.println("[UART1<-] 行超长，丢弃");
            }
        }
    }
    return capture_req;
}

bool ai_link_send_result(const AiResult& r)
{
    JsonDocument doc;
    doc["type"]       = "result";
    doc["animal"]     = r.animal_en;
    doc["confidence"] = roundf(r.confidence * 100) / 100.0f;  // 两位小数
    doc["label_cn"]   = r.label_cn;

    serializeJson(doc, s_uart);
    s_uart.print('\n');                              // 协议约定 \n 结尾

    Serial.print("[UART1->] ");                      // USB 同步打印一份，单板也能看协议
    serializeJson(doc, Serial);
    Serial.println();
    return true;
}
