#ifndef DAC_CONTROL_H
#define DAC_CONTROL_H

#include <stdint.h>

// 初始化函数
void DAC_Output_Init(void);

// 核心控制函数
void Control_DAC_Enable(uint8_t ch, uint8_t enable);

// 内部计算函数
void Calc_DAC_Buffer(uint8_t ch);

// 参数设置函数
void Set_DAC_Vpp(uint8_t ch, float Vpp);
void Set_DAC_Wave_Type(uint8_t ch, uint8_t w_type);
void Set_DAC_Freq(uint8_t ch, float freq);
void Set_DAC_Duty(uint8_t ch, uint8_t duty);

// 参数查询函数
uint32_t Get_DAC_Freq(uint8_t ch);
float Get_DAC_Vpp(uint8_t ch);
uint8_t Get_DAC_Duty(uint8_t ch);
uint8_t Get_DAC_Wave_Type(uint8_t ch);
uint8_t Get_DAC_Status(uint8_t ch);

#endif
