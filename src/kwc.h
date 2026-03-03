#ifndef KWC_H
#define KWC_H

#include "Translator/Typedef.h"

#define SAMPLING_NUM 320
#define SHOW_KWS_INFO 0// 定义后会在串口打印每个类别的概率等详细信息，便于调试

#if SAMPLING_NUM == 160
#define TRAIN_GLOBAL_MAX (1988140.11f) // 160
#define STEP_NUM 100                   // STFT时间片数
#define FRAME_LEN_MS 10
#elif SAMPLING_NUM == 320
#define TRAIN_GLOBAL_MAX (3336757.49f) // 320
#define STEP_NUM 50                    // STFT时间片数
#define FRAME_LEN_MS 20
#elif SAMPLING_NUM == 256
#define TRAIN_GLOBAL_MAX (2686234.55f) // 256 从训练输出复制
#define STEP_NUM 62                    // STFT时间片数
#endif

#define SPECTROGRAM_NUM (SAMPLING_NUM / 4)  // 单帧频谱点数

extern int16_t s_pcm_1s[SAMPLING_NUM * STEP_NUM]; // 1秒int16_t PCM缓存
extern const char *kws_class_name[];

typedef enum
{
    KWS_ERROR = -1,
    KWS_YES = 0,
    KWS_STOP,
    KWS_UNKNOWN,
    KWS_NUM
} kws_result_t;

void kws_init(void);
kws_result_t kws(void);
void kws_preprocess_pcm(void);
void kws_preprocess(const int16_t *audio_array, uint32_t audio_len, uint32_t samplerate);

#endif
