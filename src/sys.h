#ifndef __SYS_H
#define __SYS_H
#include <stdint.h>
#include "bsp_api.h"

fsp_err_t SystickInit(void);
uint32_t HAL_GetTick(void);

void HAL_SysTick_Timer_Start_us(void);
uint32_t HAL_SysTick_Timer_Stop_us(void);

#endif
