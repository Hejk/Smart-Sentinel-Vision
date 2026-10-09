#pragma once
#include <Arduino.h>
#include "config.h"

// —— 6 类识别目标（英文键与主控端动物→音频映射表一一对应）——
enum AnimalClass : uint8_t {
    AN_WILD_BOAR = 0,   // 野猪
    AN_ROE_DEER,        // 狍子
    AN_SQUIRREL,        // 松鼠
    AN_BADGER,          // 獾子
    AN_BIRD,            // 飞鸟
    AN_HUMAN,           // 人类
    AN_COUNT
};

extern const char* ANIMAL_EN[AN_COUNT];
extern const char* ANIMAL_CN[AN_COUNT];

// 一次推理的结果（字段名即协议字段）
struct AiResult {
    const char* animal_en;   // → "animal"
    const char* label_cn;    // → "label_cn"
    float       confidence;  // → "confidence" 0.0~1.0
};

// 推理入口：输入 96x96 灰度图（行优先，0~255）。
// 当前为占位实现；后续整体替换为 Edge Impulse C++ 库，接口不变
bool ai_infer_gray96(const uint8_t* gray96, size_t len, AiResult* out);

// 占位调试：强制下一次推理输出指定类别（0~5），接入真模型后自然失效
void    ai_infer_set_forced(uint8_t cls);
uint8_t ai_infer_get_forced(void);
