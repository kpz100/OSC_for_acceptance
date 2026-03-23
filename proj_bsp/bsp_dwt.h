#ifndef __BSP_DWT_H
#define __BSP_DWT_H

#include "main.h"

#define EXECUTE_WITH_MIN_PERIOD(_ms, _func)           \
    do {                                              \
        uint32_t _start_tick = BSP_DWT_GetCounter();  \
        _func;                                        \
        uint32_t _end_tick = BSP_DWT_GetCounter();    \
        float _elapsed_ms = BSP_DWT_GetDelta_us(_start_tick, _end_tick) / 1000.0f; \
        if (_elapsed_ms < (_ms)) {                    \
            BSP_DWT_Delay_us((uint32_t)(((_ms) - _elapsed_ms) * 1000));            \
        }                                             \
    } while (0)          

/* 函数声明 */
void BSP_DWT_Init(void);
void BSP_DWT_Delay_us(uint32_t us);
void BSP_DWT_Delay_ms(uint32_t ms);

/* 获取当前计数器值 (用于测量代码段运行时间) */
uint32_t BSP_DWT_GetCounter(void);
/* 计算两个计数值之间的差值，并转换为微秒 */
float BSP_DWT_GetDelta_us(uint32_t start, uint32_t end);

#endif /* __BSP_DWT_H */