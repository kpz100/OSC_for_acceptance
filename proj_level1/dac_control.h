#ifndef DAC_CONTROL_H
#define DAC_CONTROL_H

#include "main.h"

#define DAC_LENGTH 100u
#define DAC_CHANNEL_NUM 2u

#define PI2 6.283185307f
#define VERF 3.3f
#define DOUBLE_VERF (2.0f * VERF)
#define U12BIT 4095u

void DAC_Output_Init(void);

void Control_DAC_Enable(uint8_t ch, uint8_t enable);

void Calc_DAC_Buffer(uint8_t ch);

void Set_DAC_Vpp(uint8_t ch, float Vpp);
void Set_DAC_Wave_Type(uint8_t ch, uint8_t w_type);
void Set_DAC_Freq(uint8_t ch, float freq);
void Set_DAC_Duty(uint8_t ch, uint8_t duty);

uint32_t Get_DAC_Freq(uint8_t ch);
float Get_DAC_Vpp(uint8_t ch);
uint8_t Get_DAC_Duty(uint8_t ch);
uint8_t Get_DAC_Wave_Type(uint8_t ch);
uint8_t Get_DAC_Status(uint8_t ch);

#endif