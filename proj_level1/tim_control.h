#ifndef TIM_CONTROL_H
#define TIM_CONTROL_H

#include <stdint.h>

void Tim_Control_Init(void);
uint32_t Get_Tim_Freq(uint8_t ch);
void Set_Tim_Freq(uint8_t ch, uint32_t freq);
void Control_Tim_Clk(uint8_t ch, uint8_t enable);

#endif
