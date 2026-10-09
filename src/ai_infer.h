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
// 内部跑 Edge Impulse 真模型（3 类），输出映射回 6 类协议表 + unknown
bool ai_infer_gray96(const uint8_t* gray96, size_t len, AiResult* out);

// 调试覆盖：强制下一次推理输出指定类别（0~5）
void    ai_infer_set_forced(uint8_t cls);
void    ai_infer_clear_forced(void);        // 取消覆盖，恢复真模型
bool    ai_infer_forced_active(void);       // 当前是否处于覆盖状态
uint8_t ai_infer_get_forced(void);          // AN_COUNT = 未覆盖
