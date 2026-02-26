#include "sample.h"
#include "hal_data.h"
#include "kwc.h"
#include "debug_print.h"
#include "webrtc_vad/include/webrtc_vad.h"
#include "webrtc_vad/include/vad_core.h"

// 全局/static变量（复用原有变量，测试前重置）
static uint16_t adc_buf[2][SAMPLING_NUM] = {0}; // 模拟ADC采样的音频数据数组
volatile uint8_t adc_buf_num = 0;
static int16_t adc_temp_buf[SAMPLING_NUM] = {0};

volatile bool g_detect_frame_flag = false;    // adc单帧采样完成后置位，检测当前帧是否有语音
volatile bool g_speech_detected_flag = false; // 检测到语音后置位，连续采集STEP_NUM后复位

volatile uint32_t adc5_callback_count = 0;
volatile uint32_t adc_frame_index = 0;
static volatile bool adc_sample_cplt = false;

static void detect_frame(void);
static void ADCWaitConvCplt(void);
static int webrtc_vad_init(void);

VadInst *vad_inst = NULL;

void sample_init(void)
{
    /* 打开ADC设备完成通用初始化 */
    fsp_err_t err = g_adc5.p_api->open(g_adc5.p_ctrl, g_adc5.p_cfg);
    assert(FSP_SUCCESS == err);
    /* 配置ADC指令的通道完成初始化 */
    err = g_adc5.p_api->scanCfg(g_adc5.p_ctrl, g_adc5.p_channel_cfg);
    assert(FSP_SUCCESS == err);
    /* 打开ELC设备完成初始化 */
    err = g_elc.p_api->open(g_elc.p_ctrl, g_elc.p_cfg);
    assert(FSP_SUCCESS == err);
    /* 使能ELC的连接功能 */
    err = g_elc.p_api->enable(g_elc.p_ctrl);
    // assert(FSP_SUCCESS == err);
    // /* 打开DMA设备完成初始化 */
    // err = g_transfer0.p_api->open(g_transfer0.p_ctrl, g_transfer0.p_cfg);
    // assert(FSP_SUCCESS == err);
    // /* 使能DMAC的ELC触发源 */
    // err = g_transfer0.p_api->enable(g_transfer0.p_ctrl);
    // assert(FSP_SUCCESS == err);
    /* 打开定时器设备完成初始化 */
    err = g_timer0.p_api->open(g_timer0.p_ctrl, g_timer0.p_cfg);
    assert(FSP_SUCCESS == err);
    /* 使能ADC的转换功能 */
    err = g_adc5.p_api->scanStart(g_adc5.p_ctrl);
    assert(FSP_SUCCESS == err);

    // 初始化VAD
    int ret = webrtc_vad_init();
    assert(ret != -1);
    ret = WebRtcVad_set_mode(vad_inst, 2);
    assert(ret == 0);
}

// volatile uint32_t dma_count = 0;
void dma0_callback(transfer_callback_args_t *p_args)
{
    (void)p_args;
    // print("dma transfer done callback!\r\n");
    // dma_count++;
    // if(dma_count >= 128)
    // {
    //     dma_count = 0;
    //     adc_sample_cplt = true;
    // }
}

void adc5_callback(adc_callback_args_t *p_args)
{
    (void)p_args;
    uint16_t *adc_data = adc_buf[adc_buf_num];
    g_adc5.p_api->read(g_adc5.p_ctrl, ADC_CHANNEL_5, &adc_data[adc5_callback_count]);
    adc5_callback_count++;

    if (adc5_callback_count >= SAMPLING_NUM)
    {
        adc5_callback_count = 0;
        adc_buf_num ^= 1;           // 切换写入 buffer
        g_detect_frame_flag = true; // 通知主循环有完整 frame 可检测/搬运
    }
}

void sample_start(void)
{
    fsp_err_t err = FSP_SUCCESS;
    print("start sampling...\r\n");

    // 重置所有相关全局变量
    adc_sample_cplt = false;
    adc5_callback_count = 0;
    adc_frame_index = 0;
    g_detect_frame_flag = false;
    g_speech_detected_flag = false;

    /* 开启定时器触发ADC采样 */
    err = g_timer0.p_api->start(g_timer0.p_ctrl);
    assert(FSP_SUCCESS == err);

    ADCWaitConvCplt();
    /* 采样结束后关闭定时器 */
    err = g_timer0.p_api->stop(g_timer0.p_ctrl);
    assert(FSP_SUCCESS == err);
}

static void detect_frame(void)
{
    if (!g_detect_frame_flag)
    {
        return;
    }
    g_detect_frame_flag = false;

    uint16_t *adc_data = adc_buf[adc_buf_num ^ 1]; // 使用双缓冲切换前的数据

    // 依据简单的阈值判断是否有语音出现，（后续可借鉴VAD算法）
    for (uint32_t i = 0; i < SAMPLING_NUM; i++)
    {
        int16_t abs_val = (int16_t)((adc_data[i] > 0) ? adc_data[i] : -adc_data[i]);
        if (abs_val - 1525 > 250)
        {
            g_speech_detected_flag = true;
            break;
        }
    }
}

static void ADCWaitConvCplt(void)
{
    while (!adc_sample_cplt)
    {
        // print("adc_sample_cplt is false!\r\n");
        // print("adc_test_flag is %d!\r\n", adc_test_flag);

        if (g_detect_frame_flag)
        {
            // print("g_detect_frame_flag is true!\r\n");

            g_detect_frame_flag = false;
            uint16_t *ready_buf = adc_buf[adc_buf_num ^ 1];

            // 消除DC偏置
            for (uint32_t i = 0; i < SAMPLING_NUM; i++)
            {
                adc_temp_buf[i] = (int16_t)ready_buf[i] - 1525;   // 1.25v 左右的DC偏置，这里直接取1525
            }

            // 如果尚未进入采集状态则检测起始帧
            if (!g_speech_detected_flag)
            {
                // for (uint32_t i = 0; i < SAMPLING_NUM; i++)
                // {
                //     int16_t v = (int16_t)ready_buf[i] - 1525;
                //     if ((v > 250) || (v < -250))
                //     {
                //         g_speech_detected_flag = true;
                //         print("speech detected!\r\n");

                //         adc_frame_index = 0; // start at 0
                //         break;
                //     }
                // }
                int ret = WebRtcVad_Process(vad_inst, 16000, adc_temp_buf, SAMPLING_NUM);
                assert(ret != -1);
                if (ret == 1)
                {
                    g_speech_detected_flag = true;
                    print("speech detected!\r\n");

                    adc_frame_index = 0; // start at 0
                    break;
                };
            }

            if (g_speech_detected_flag)
            {
                // print("g_speech_detected_flag is true!\r\n");

                // 拷贝
                for (uint32_t i = 0; i < SAMPLING_NUM; i++)
                {
                    s_pcm_1s[adc_frame_index * SAMPLING_NUM + i] = adc_temp_buf[i];
                }
                adc_frame_index++;
                if (adc_frame_index >= STEP_NUM)
                {
                    adc_sample_cplt = true;
                    g_speech_detected_flag = false;
                    // keep adc_frame_index for later resets if needed
                }
            }
        }
    }
    adc_sample_cplt = false;
    print("1s audio sample complete!\r\n");
}

static int webrtc_vad_init(void)
{
    vad_inst = WebRtcVad_Create_static();
    return WebRtcVad_Init(vad_inst);
}