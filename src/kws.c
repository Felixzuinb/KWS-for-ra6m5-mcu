#include "kwc.h"
#include "arm_math.h"             // CMSIS-DSP库（FFT依赖）
#include "mixed_radix_2_5_f32.h" // 混合基FFT（支持160/320点）
#include "debug_print.h"
#include "sys.h"


int16_t s_pcm_1s[SAMPLING_NUM * STEP_NUM]; // 1秒int16_t PCM缓存

static float_t s_spectrogram[STEP_NUM][SPECTROGRAM_NUM];                      // 最终频谱图（DNN输入）
static float_t s_fft_buf[SAMPLING_NUM];                                       // FFT输入缓冲区
extern TsOUT *dnn_compute(TsIN *serving_default_input_1_0, TsInt *errorcode); // DNN推理函数声明
static float_t fft_complex[SAMPLING_NUM * 2] = {0};                           // FFT复数输入缓冲区（实部+虚部）
static float_t fft_mag[SAMPLING_NUM / 2] = {0};                               // FFT幅值输出缓冲区（128点）
static float_t dnn_input[STEP_NUM * SPECTROGRAM_NUM];                         // DNN输入缓冲区（1维化的频谱图）


static void convert_and_print_spectrogram(void);

void kws_preprocess(const int16_t *audio_array, uint32_t audio_len, uint32_t samplerate)
{
    // 步骤1：找到最大值进行幅度缩放对齐
    int16_t max_val = 0;
    for (uint32_t i = 0; i < audio_len; i++)
    {
        int16_t abs_val = (int16_t)((audio_array[i] > 0) ? audio_array[i] : -audio_array[i]);
        max_val = (abs_val > max_val) ? abs_val : max_val;
    }
    int16_t threashold = (int16_t)(max_val / 4); // 5000 / 20000 * max_val, 以对齐后的±5000为阈值，找到语音开始位置

    // 步骤2：裁剪，语音数据左对齐，并末尾补0
    uint32_t start_num = 0;
    for (uint32_t i = 0; i < audio_len; i++)
    {
        if (audio_array[i] > threashold || audio_array[i] < -threashold)
        { // 以±5000为阈值，找到语音开始位置
            start_num = i;
            break;
        }
    }

    if (max_val == 0)
    {
        return; // 避免除0错误，且全0数据无需处理
    }

    for (size_t i = 0; i < audio_len - start_num; i++)
    {
        s_pcm_1s[i] = (int16_t)(audio_array[start_num + i] * (20000.0f / max_val)); // 幅度对齐到±20000; // 从start_num开始处理
    }
    for (size_t i = audio_len - start_num; i < audio_len; i++)
    {
        s_pcm_1s[i] = 0; // 不足部分填0
    }
}

void kws_preprocess_pcm(void)
{
    // kws_preprocess_frame已经检测了语音开始位置，并将数据左对齐到s_pcm_1s开头了，这里只需要做幅度缩放对齐就行了

    // 步骤1：找到最大值进行幅度缩放对齐
    int16_t max_val = 0;
    for (uint32_t i = 0; i < SAMPLING_NUM * STEP_NUM; i++)
    {
        int16_t abs_val = (int16_t)((s_pcm_1s[i] > 0) ? s_pcm_1s[i] : -s_pcm_1s[i]);
        max_val = (abs_val > max_val) ? abs_val : max_val;
    }
    if (max_val == 0)
    {
        return; // 避免除0错误，且全0数据无需处理
    }

    for (size_t i = 0; i < SAMPLING_NUM * STEP_NUM; i++)
    {
        s_pcm_1s[i] = (int16_t)(s_pcm_1s[i] * (20000.0f / max_val)); // 幅度对齐到±20000
    }
}

void kws_init(void)
{
    init_mixed_radix_2_5_fft_160_320(SAMPLING_NUM); // 初始化FFT twiddle因子
}

void kws()
{
    // -------------------------- 步骤1：初始化+打印测试开始信息 --------------------------
    TsInt errorcode = 0;
    print("[KWS] start\r\n");

    uint32_t process_time = HAL_GetTick();
    uint32_t total_time = 0;

// -------------------------- 步骤3：STFT（短时FFT）+ 频谱图生成（复刻fft_execute逻辑） --------------------------

// 3.2 初始化FFT实例（ARM CMSIS-DSP）
#if SAMPLING_NUM == 256
    // 256是2的幂次方，使用标准CFFT实例
    arm_cfft_instance_f32 s_fft_inst;
    arm_cfft_init_f32(&s_fft_inst, SAMPLING_NUM);
#elif SAMPLING_NUM == 160 || SAMPLING_NUM == 320
    // // 160和320使用mixed_25 FFT实例
    // arm_fft_mixed_25_instance_f32 s_fft_inst;
    // arm_fft_mixed_25_init_f32(&s_fft_inst, SAMPLING_NUM);
#endif

    // float fft_max_val = 0.0f;
    // 3.3 逐时间片执行FFT+降维+归一化
    for (uint8_t jj = 0; jj < STEP_NUM; jj++)
    {
        // 子步骤1：int32_t PCM → float_t（FFT输入要求）
        for (uint16_t ii = 0; ii < SAMPLING_NUM; ii++)
        {
            s_fft_buf[ii] = (float_t)s_pcm_1s[(jj * SAMPLING_NUM) + ii];
        }

        // 子步骤2：执行FFT
        // 构造复数输入（所有采样点数都需要）
        for (uint16_t ii = 0; ii < SAMPLING_NUM; ii++)
        {
            fft_complex[ii * 2] = s_fft_buf[ii]; // 实部
            fft_complex[ii * 2 + 1] = 0.0f;      // 虚部
        }

// 根据采样点数选择FFT函数
#if SAMPLING_NUM == 256
        // 256是2的幂次方，使用标准CFFT函数
        arm_cfft_f32(&s_fft_inst, fft_complex, 0, 1);
#elif SAMPLING_NUM == 160 || SAMPLING_NUM == 320
        // 160和320使用mixed_25 FFT函数
        // arm_fft_mixed_25_f32(&s_fft_inst, fft_complex);
        mixed_radix_2_5_fft_160_320(fft_complex, SAMPLING_NUM);
#endif

        // 子步骤4：计算FFT幅值（取前N/2点，共轭对称）
        // float_t fft_mag[SAMPLING_NUM / 2] = {0};
        arm_cmplx_mag_f32(fft_complex, fft_mag, SAMPLING_NUM / 2);

        // 子步骤5：降维（相邻平均）+ 缩放+归一化
        // fft_mag[0] = 0; // 直流分量置0

        // 根据SAMPLING_NUM执行相应的降维
        // 256点FFT：128点→64点降维（SAMPLING_NUM/4）
        // 160点FFT：80点→40点降维（SAMPLING_NUM/4）
        // 320点FFT：160点→80点降维（SAMPLING_NUM/4）
        for (uint8_t kk = 0; kk < (SAMPLING_NUM / 2); kk += 2)
        {
            float_t ave = (fft_mag[kk] + fft_mag[kk + 1]) / 2.0f; // 相邻平均
            s_spectrogram[jj][kk / 2] = ave;
            // fft_max_val = (ave > fft_max_val) ? ave : fft_max_val;
        }
    }
    // 训练对齐缩放
    for (uint8_t jj = 0; jj < STEP_NUM; jj++)
    {
        for (uint8_t kk = 0; kk < SPECTROGRAM_NUM; kk++)
        {
            s_spectrogram[jj][kk] = s_spectrogram[jj][kk] / TRAIN_GLOBAL_MAX;
        }
    }

    // convert_and_print_spectrogram();

    // print("[KWS] STFT done: get %d x %dspectrum, date 0~1\r\n", STEP_NUM, SPECTROGRAM_NUM);

#if 1
    // -------------------------- 步骤4：DNN推理 + 结果打印 --------------------------
    // 将二维频谱图转为一维数组（适配dnn_compute输入格式）
    // float_t dnn_input[STEP_NUM * SPECTROGRAM_NUM];
    for (uint8_t i = 0; i < STEP_NUM; i++)
    {
        for (uint8_t j = 0; j < SPECTROGRAM_NUM; j++)
        {
            dnn_input[i * SPECTROGRAM_NUM + j] = s_spectrogram[i][j];
        }
    }

    uint32_t stft_time = HAL_GetTick() - process_time;
    total_time += stft_time;
    process_time = HAL_GetTick();

    // 调用DNN推理函数
    TPrecision *pred_result = (TsOUT *)(intptr_t)dnn_compute(dnn_input, &errorcode);
    if (errorcode != 0 || pred_result == NULL)
    {
        print("[KWS] DNN error! error code: %d\r\n", errorcode);
        return;
    }

    uint32_t dnn_time = HAL_GetTick() - process_time;
    total_time += dnn_time;

    // 解析推理结果：找最大概率类别
    // 打印最终结果
    const char *class_name[] = {"go", "left", "right", "stop", "yes"}; // 按训练类别对应

    uint8_t max_class = 0;
    float_t max_prob = 0.0f;
    uint8_t class_num = 5; // 分类数和训练模型一致
    for (uint8_t i = 0; i < class_num; i++)
    {
        if (pred_result[i] > max_prob)
        {
            max_prob = pred_result[i];
            max_class = i;
        }
        print("[KWS] type %d %s:0.%d\r\n", i, class_name[i], (int)(pred_result[i] * 10000));
    }

    print("[KWS] inference done! class: %s (class %d), 0.%d\r\n",
          class_name[max_class], max_class, (int)(max_prob * 10000));

    print("\r\n[KWS] STFT & normalization time: %ums\r\n", stft_time);
    print("[KWS] dnn inference time: %ums\r\n", dnn_time);
    print("[KWS] kws time: %ums\r\n", total_time);

    // -------------------------- 步骤5：重置变量（避免影响后续测试） --------------------------
    memset(s_pcm_1s, 0, sizeof(s_pcm_1s));
    memset(s_spectrogram, 0, sizeof(s_spectrogram));
    memset(s_fft_buf, 0, sizeof(s_fft_buf));
    print("[KWS] kws done, variables reset\r\n\r\n");
#endif
}

/**
 * 将浮点类型的语谱图数据转化为0-255范围的整数并输出
 * 用于通过串口发送数据到主机进行绘制
 */
static void convert_and_print_spectrogram(void)
{
    // 找到整个语谱图的最大值和最小值，用于归一化
    float_t min_val = s_spectrogram[0][0];
    float_t max_val = s_spectrogram[0][0];

    for (uint32_t i = 0; i < STEP_NUM; i++)
    {
        for (uint32_t j = 0; j < SPECTROGRAM_NUM; j++)
        {
            float_t val = s_spectrogram[i][j];
            if (val < min_val)
            {
                min_val = val;
            }
            if (val > max_val)
            {
                max_val = val;
            }
        }
    }

    // 计算值范围
    float_t value_range = max_val - min_val;
    if (value_range == 0)
    {
        value_range = 1.0f; // 避免除零错误
    }

    // 开始输出
    print("[");

    for (uint32_t i = 0; i < STEP_NUM; i++)
    {
        if (i > 0)
        {
            print("\r\n, ");
        }
        print("[");

        for (uint32_t j = 0; j < SPECTROGRAM_NUM; j++)
        {
            if (j > 0)
            {
                print(", ");
            }

            // 归一化到0-1范围
            float_t normalized_value = (s_spectrogram[i][j] - min_val) / value_range;

            // 映射到0-255范围
            uint8_t int_value = (uint8_t)(normalized_value * 255.0f);

            // 确保值在0-255范围内
            if (int_value < 0)
            {
                int_value = 0;
            }
            else if (int_value > 255)
            {
                int_value = 255;
            }

            // 输出值
            print("%d", int_value);
        }

        print("]");
    }

    // 结束输出
    print("]");
}