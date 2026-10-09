#include "ai_infer.h"

const char* ANIMAL_EN[AN_COUNT] =
    { "wild_boar", "roe_deer", "squirrel", "badger", "bird", "human" };
const char* ANIMAL_CN[AN_COUNT] =
    { "野猪", "狍子", "松鼠", "獾子", "飞鸟", "人类" };

static uint8_t s_forced = AN_WILD_BOAR;

void ai_infer_set_forced(uint8_t cls) { if (cls < AN_COUNT) s_forced = cls; }
uint8_t ai_infer_get_forced(void)     { return s_forced; }

// ============================================================
//  ★ 占位推理（stub）—— 后续替换为 Edge Impulse C++ 库
//
//  替换步骤（对外接口不变，main / ai_link / cam_module 都不用改）：
//   1) EI 平台导出 Arduino library（C++ 版），解压到本项目 lib/ 下，
//      例如 lib/ei_smart_sentinel/
//   2) 本文件顶部 #include <ei_smart_sentinel_inferencing.h>
//   3) 写 signal 回调，把 96x96 灰度转成 0~1 浮点喂给模型：
//        static int ei_feed(size_t offset, size_t length, float* out) {
//            for (size_t i = 0; i < length; i++)
//                out[i] = gray96[offset + i] / 255.0f;
//            return 0;
//        }
//   4) 推理调用：
//        ei::signal_t signal; signal.get_data = &ei_feed;
//        ei_impulse_result_t result;
//        run_classifier(&signal, &result, false);
//   5) 取 result.classification[] 里 value 最大的 label 映射到
//      AnimalClass（EI 训练时的 label 建议直接用 wild_boar 等英文键，
//      就能和 ANIMAL_EN 表直接对上），删掉下面的占位逻辑
// ============================================================
bool ai_infer_gray96(const uint8_t* gray96, size_t len, AiResult* out)
{
    if (!gray96 || !out || len < (size_t)(AI_IMG_W * AI_IMG_H)) return false;

    // —— 占位逻辑开始（TODO: 用 EI run_classifier 替换）——
    // 粗统计一下画面亮度，留作后续"画面全黑/镜头遮挡"自检的种子
    uint32_t bright_sum = 0;
    for (size_t i = 0; i < len; i += 64) bright_sum += gray96[i];
    float avg_bright = bright_sum / (float)(len / 64 + 1);
    (void)avg_bright;   // 接入真模型后在日志里输出它

    out->animal_en  = ANIMAL_EN[s_forced];
    out->label_cn   = ANIMAL_CN[s_forced];
    out->confidence = 0.85f;
    // —— 占位逻辑结束 ——

    return true;
}
