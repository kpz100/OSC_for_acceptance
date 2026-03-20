#include "bsp_dwt.h"

void BSP_DWT_Init(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

uint32_t BSP_DWT_GetCounter(void)
{
    return DWT->CYCCNT;
}

void BSP_DWT_Delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (SystemCoreClock / 1000000);
    while ((DWT->CYCCNT - start) < ticks);
}

void BSP_DWT_Delay_ms(uint32_t ms)
{
    BSP_DWT_Delay_us(ms * 1000);
}

float BSP_DWT_GetDelta_us(uint32_t start, uint32_t end)
{
    uint32_t delta = (end >= start) ? (end - start) : (0xFFFFFFFF - start + end);
    return (float)delta / (SystemCoreClock / 1000000.0f);
}
