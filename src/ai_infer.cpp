#include "ai_infer.h"

// —— Edge Impulse 生成的推理库（zip 在 edge-impulse/ 下解压为 model-v2/）——
// platformio.ini 用 lib_extra_dirs 指过去，library.json 只编译
// classifier/dsp/porting(espressif)/tensorflow/tflite-model 五个子树
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"

// —— 对外协议类别表（主控板的查表基准，顺序勿动）——
const char* ANIMAL_EN[AN_COUNT] =
    { "wild_boar", "roe_deer", "squirrel", "badger", "bird", "human" };
const char* ANIMAL_CN[AN_COUNT] =
    { "野猪", "狍子", "松鼠", "獾子", "飞鸟", "人类" };

// ============================================================
//  真实推理（Edge Impulse run_classifier）
//  模型：my train model #1133771 · 96x96 图像 · int8 量化
//  输出 3 类：bird(飞鸟) / unknown(未知) / wildboar(野猪，无下划线)
//  tensor arena 约 250KB，为 SDK 内静态 BSS 数组，不占堆
// ============================================================

static int8_t s_forced = -1;    // -1 = 跟真模型；0~5 = 调试覆盖

void ai_infer_set_forced(uint8_t cls) { if (cls < AN_COUNT) s_forced = (int8_t)cls; }
void ai_infer_clear_forced(void)      { s_forced = -1; }
bool ai_infer_forced_active(void)     { return s_forced >= 0; }
uint8_t ai_infer_get_forced(void)     { return s_forced < 0 ? (uint8_t)AN_COUNT : (uint8_t)s_forced; }

// —— EI 标签 → 协议类别映射（EI 上 wildboar 没有下划线！）——
struct EiMap { const char *ei_label; const char *en; const char *cn; };
static const EiMap EI_MAP[] = {
    { "wildboar", "wild_boar", "野猪" },
    { "bird",     "bird",      "飞鸟" },
};
static const size_t EI_MAP_N = sizeof(EI_MAP) / sizeof(EI_MAP[0]);

static const float EI_CONF_MIN = 0.40f;   // top-1 低于它 → 一律按 unknown 上报

static const uint8_t *s_gray = nullptr;   // 当前帧指针（喂给 signal 回调）

// EI 图像信号的约定格式（见 ei_run_dsp.h extract_image_features）：
//   total_length = W*H，每个元素是 float，位型为打包的 0xRRGGBB；
//   SDK 内部解包 r=(pixel>>16)&0xff 等，RGB 模型输出 3 通道特征。
// 模型按彩色图训练，灰度摄像头帧复刻成 R=G=B 喂入；
// float 24 位尾数可精确表示 0xFFFFFF，位型无损。
static int ei_feed_gray96(size_t offset, size_t length, float *out)
{
    if (!s_gray) return -1;
    for (size_t i = 0; i < length; i++) {
        uint32_t g = s_gray[offset + i];
        out[i] = (float)((g << 16) | (g << 8) | g);
    }
    return 0;
}

bool ai_infer_gray96(const uint8_t *gray96, size_t len, AiResult *out)
{
    if (!gray96 || !out || len < (size_t)(AI_IMG_W * AI_IMG_H)) return false;

    // —— 调试覆盖：USB 命令 0~5 设置，o 恢复真模型 ——
    if (s_forced >= 0) {
        out->animal_en  = ANIMAL_EN[s_forced];
        out->label_cn   = ANIMAL_CN[s_forced];
        out->confidence = 0.85f;
        return true;
    }

    s_gray = gray96;
    ei::signal_t signal;
    signal.total_length = (size_t)EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
    signal.get_data     = &ei_feed_gray96;

    ei_impulse_result_t result = { 0 };
    uint32_t t0 = millis();
    EI_IMPULSE_ERROR err = run_classifier(&signal, &result, false /*debug*/);
    s_gray = nullptr;                       // 推理完立刻断开，防误用

    if (err != EI_IMPULSE_OK) {
        Serial.printf("[AI] run_classifier 失败 err=%d（内存不足看 err=11/13）\n", (int)err);
        return false;
    }
    uint32_t infer_ms = millis() - t0;

    // —— 取 top-1 ——
    float best = 0.0f;
    size_t best_i = 0;
    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        if (result.classification[i].value > best) {
            best = result.classification[i].value;
            best_i = i;
        }
    }
    const char *ei_label = result.classification[best_i].label;

    // —— 映射到协议类别；低于 0.4 或本来就是 unknown → "unknown" ——
    out->animal_en = "unknown";
    out->label_cn  = "未知";
    if (best >= EI_CONF_MIN) {
        for (size_t i = 0; i < EI_MAP_N; i++) {
            if (strcmp(EI_MAP[i].ei_label, ei_label) == 0) {
                out->animal_en = EI_MAP[i].en;
                out->label_cn  = EI_MAP[i].cn;
                break;
            }
        }
    }
    out->confidence = best;

    Serial.printf("[AI] 模型输出: %s=%.0f%% → %s(%s)  推理%lums\n",
                  ei_label, best * 100.0f,
                  out->animal_en, out->label_cn, (unsigned long)infer_ms);
    return true;
}
