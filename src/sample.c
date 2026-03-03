#include "sample.h"
#include "hal_data.h"
#include "kwc.h"
#include "debug_print.h"
#include "webrtc_vad/include/webrtc_vad.h"
#include "webrtc_vad/include/vad_core.h"
#include "sys.h"

#define VAD_MODE 3   // 0-3, 决定VAD的激进程度，数值越大代表越激进，误报率越低但漏报率越高

// 全局/static变量（复用原有变量，测试前重置）
static uint16_t adc_buf[2][SAMPLING_NUM] = {0};     // 模拟ADC采样的音频数据数组
static int16_t adc_temp_buf[SAMPLING_NUM] = {0};    // 临时缓冲区，为了去除DC偏移，暂时给webrtc_vad使用
volatile uint8_t adc_buf_num = 0;                   // 当前正在使用的缓冲区索引，0或1

volatile bool g_detect_frame_flag = false;    // adc单帧采样完成后置位，检测当前帧是否有语音
volatile bool g_speech_detected_flag = false; // 检测到语音后置位，连续采集STEP_NUM后复位

volatile uint32_t adc_frame_index = 0;
static volatile bool adc_sample_cplt = false;

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
    assert(FSP_SUCCESS == err);
    /* 打开DMA设备完成初始化 */
    err = g_transfer0.p_api->open(g_transfer0.p_ctrl, g_transfer0.p_cfg);
    assert(FSP_SUCCESS == err);
    /* 使能DMAC的ELC触发源 */
    err = g_transfer0.p_api->enable(g_transfer0.p_ctrl);
    assert(FSP_SUCCESS == err);
    /* 打开定时器设备完成初始化 */
    err = g_timer0.p_api->open(g_timer0.p_ctrl, g_timer0.p_cfg);
    assert(FSP_SUCCESS == err);
    /* 使能ADC的转换功能 */
    err = g_adc5.p_api->scanStart(g_adc5.p_ctrl);
    assert(FSP_SUCCESS == err);

    // 初始化VAD
    int ret = webrtc_vad_init();
    assert(ret != -1);
    ret = WebRtcVad_set_mode(vad_inst, VAD_MODE);
    assert(ret == 0);
}

// volatile uint32_t dma_count = 0;
void dma0_callback(transfer_callback_args_t *p_args)
{
    (void)p_args;
    // 切换缓冲区
    adc_buf_num ^= 1;
    g_detect_frame_flag = true; // 通知主循环有完整 frame 可检测/搬运

    g_transfer0.p_api->close(g_transfer0.p_ctrl);
    transfer_info_t *transfer_info = g_transfer0.p_cfg->p_info;

    transfer_info->p_dest = adc_buf[adc_buf_num];

    g_transfer0.p_api->open(g_transfer0.p_ctrl, g_transfer0.p_cfg);
    g_transfer0.p_api->enable(g_transfer0.p_ctrl);
}

void sample_start(void)
{
    fsp_err_t err = FSP_SUCCESS;
    print("start sampling...\r\n");

    // 重置所有相关全局变量
    adc_sample_cplt = false;
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

static void ADCWaitConvCplt(void)
{
    while (!adc_sample_cplt)
    {
        if (g_detect_frame_flag)
        {
            g_detect_frame_flag = false;
            uint16_t *ready_buf = adc_buf[adc_buf_num ^ 1];

            // 消除DC偏置
            for (uint32_t i = 0; i < SAMPLING_NUM; i++)
            {
                adc_temp_buf[i] = (int16_t)ready_buf[i] - 1525; // 1.25v 左右的DC偏置，这里直接取1525
            }

            // 每一帧均检测是否有语音
            HAL_SysTick_Timer_Start_us();
            int ret = WebRtcVad_Process(vad_inst, 16000, adc_temp_buf, SAMPLING_NUM);
            assert(ret != -1);
            uint32_t process_time = HAL_SysTick_Timer_Stop_us();

            // 如果尚未进入采集状态则当前帧检测到语音后进入采集状态
            if (!g_speech_detected_flag)
            {
                // 只有在非采集状态下， vad检测结果才会影响是否进入采集状态
                if (ret == 1)
                {
                    g_speech_detected_flag = true;
                    print("speech detected!\r\n");
                    print("VAD processing time: %uus\r\n", process_time);
                    adc_frame_index = 0; // start at 0
                };
            }
            else    // 持续帧采集状态
            {
                // 拷贝
                // for (uint32_t i = 0; i < SAMPLING_NUM; i++)
                // {
                //     s_pcm_1s[adc_frame_index * SAMPLING_NUM + i] = adc_temp_buf[i];
                // }
                memcpy(&s_pcm_1s[adc_frame_index * SAMPLING_NUM], adc_temp_buf, SAMPLING_NUM * sizeof(int16_t));
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



void vad_test(void)
{
    static int16_t test[SAMPLING_NUM] = {0};

    // 初始化VAD
    int ret = webrtc_vad_init();
    assert(ret != -1);
    ret = WebRtcVad_set_mode(vad_inst, 0);
    assert(ret == 0);

    ret = WebRtcVad_Process(vad_inst, 16000, test, 320);
    assert(ret != -1);

    if (ret == 1)
    {
        print("speech detected!\r\n");
    };
}

int webrtc_vad_mode_change(void)
{
    static int mode = VAD_MODE;
    mode = (mode + 1) % 4; // Cycle through modes 0-3
    int ret = WebRtcVad_set_mode(vad_inst, mode);
    assert(ret == 0);
    return mode;
}
