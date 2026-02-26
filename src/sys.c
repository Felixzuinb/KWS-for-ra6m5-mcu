#include "sys.h"

#include <core_cm33.h>

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

static uint32_t systick_val = 0;
static uint32_t start_tick = 0;
void HAL_SysTick_Timer_Start_us(void)
{
    systick_val = SysTick->VAL;
    start_tick = dwTick;
}

uint32_t HAL_SysTick_Timer_Stop_us(void)
{
    uint32_t elapsed_ticks = dwTick - start_tick;
    uint32_t elapsed_us = (elapsed_ticks * 1000U) + ((systick_val - SysTick->VAL) * 1000U) / R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_CPUCLK);
    return elapsed_us;
}