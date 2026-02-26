#include "sys.h"

static volatile uint32_t dwTick = 0;

fsp_err_t SystickInit(void)
{
    uint32_t uwSysclk = R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_CPUCLK);
    if(SysTick_Config(uwSysclk / 1000U) != 0)
    {
        return FSP_ERR_ASSERTION;
    }
    return FSP_SUCCESS;
}

void SysTick_Handler(void)
{
    dwTick++;

    extern void key_process_jitter(uint32_t tick);
    key_process_jitter(dwTick);
}

uint32_t HAL_GetTick(void)
{
    return dwTick;
}