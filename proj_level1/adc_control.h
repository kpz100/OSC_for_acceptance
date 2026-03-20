#ifndef ADC_CONTROL_H
#define ADC_CONTROL_H

#include <stdint.h>
#include "adc.h"

// 初始化函数
void ADC_FFT_Init(void);

// 核心控制函数
void Control_ADC_Enable(uint8_t ch, uint8_t enable);

// FFT计算和采样
uint8_t Calc_Comp_FFT_Ampl(uint8_t ch);
uint8_t Sample_Until_FFT(uint8_t ch);  // 返回：0=重新采样，1=低频过采样，2=高频ETS采样

// 内部计算函数
uint8_t Calc_Vpp8(uint8_t ch);
uint32_t Calc_Rising_Edge_Pos(uint8_t ch);
void Process_Show_Buffer(uint32_t ch, uint8_t * array, uint32_t length);

// DMA回调处理
void Callback_Control(uint8_t ch, uint32_t adc_pos, uint32_t length);

// 中断回调函数
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc);
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc);

// 状态和参数查询
uint8_t Get_ADC_Running_Status(uint8_t ch);
uint8_t Get_Vpp_8(uint8_t ch);
float Get_FFT_Freq(uint8_t ch);
float Get_Sample_Freq(uint8_t ch);
uint8_t Get_ADC_Flag(uint8_t ch);

#endif
