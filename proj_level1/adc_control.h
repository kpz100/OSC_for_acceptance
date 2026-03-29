#ifndef ADC_CONTROL_H
#define ADC_CONTROL_H

#include "main.h"
#include "adc.h"

#ifndef abs
#define abs(x) ((x) > 0) ? (x) : -(x)
#endif

#define FFT_LENGTH 1024u
#define MAG_LENGTH (FFT_LENGTH / 2)
#define SHOW_LENGTH 2048u
#define ADC_LENGTH 1024u
#define HALF_ADC_LENGTH (ADC_LENGTH / 2)
#define ADC_CHANNEL_NUM 2u

#define MAX_FFT_AMP_ERROR 0.05f
#define SAMPLE_FREQ_SHIFT 100000u
#define SHIFT_MAX_RANK 360u

#define ORIGINAL_SAMPLE_FREQ 3600000.0f
#define MAX_OVER_SAMPLE_FREQ 100000.0f
#define OVER_SAMPLE_RATE 100.0f
#define ETS_SAMPLE_RATE 100.0f

#define RW_TARGET_NONE 0u
#define RW_TARGET_FFT 1u
#define RW_TARGET_SHOW 2u

void ADC_FFT_Init(void);
void Control_ADC_Enable(uint8_t ch, uint8_t enable);

uint8_t Calc_Comp_FFT_Ampl(uint8_t ch);
void Set_Sample_Freq(uint8_t ch, uint32_t freq);

uint8_t Calc_Vpp8(uint8_t ch);
uint32_t Calc_Rising_Edge_Pos(uint8_t ch);
void Process_Show_Buffer(uint32_t ch, uint8_t * array, uint32_t length, uint8_t clear_after);
uint8_t* Get_Show_Buffer(uint8_t ch);

void Callback_Control(uint8_t ch, uint32_t adc_wpos, uint32_t length);

uint8_t Get_Vpp_8(uint8_t ch);
float Get_FFT_Freq(uint8_t ch);
float Get_Sample_Freq(uint8_t ch);
uint8_t Get_ADC_Flag(uint8_t ch);
void Clear_ADC_Flag(uint8_t ch, uint8_t next_target_type);

#endif