#include "dac_control.h"
#include "adc.h"
#include "tim_control.h"

#include "dac.h"
#include <stdint.h>
#include <string.h>
#include <math.h>

#define DAC_LENGTH 100u
#define DAC_CHANNEL_NUM 2u

#define PI2 6.283185307f
#define VERF 3.3f
#define DOUBLE_VERF (2.0f * VERF)
#define U12BIT 4095u

typedef enum { 
    SIN_WAVE = 0, 
    SQU_WAVE, 
    TRI_WAVE
} Wave_Type;

typedef struct {
    uint16_t dac_buffer[DAC_LENGTH]; 
    Wave_Type wave_type;                    
    uint8_t duty_cycle;                   
    uint32_t freq;                         
    float Vpp;
    uint8_t status;                          
} DAC_Struct;

static DAC_Struct ch_dac_config[DAC_CHANNEL_NUM] __attribute__((section(".bss.ARM.__at_0x30001800"))) __ALIGNED(32);

static uint8_t inline Switch_Channel_Input(uint8_t ch) {
	if (ch == 1 || ch == 2) {
		return (ch - 1);
	}
	return 0;
}

void DAC_Output_Init(void) {
    memset(ch_dac_config, 0, sizeof(DAC_Struct) * DAC_CHANNEL_NUM);
    for (int i = 0; i < DAC_CHANNEL_NUM; i++) {
        ch_dac_config[i].duty_cycle = 50;
        ch_dac_config[i].freq = 1000;
        ch_dac_config[i].Vpp = 3.0f;
    }
    Control_Tim_Clk(1, 0);
	HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1);
	Control_Tim_Clk(2, 0);
	HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_2);
}

void Control_DAC_Enable(uint8_t ch, uint8_t enable) {
    if (ch != 1 && ch != 2) return;
    uint8_t sch = Switch_Channel_Input(ch);
    if (enable) {
        Calc_DAC_Buffer(ch);
        if (ch == 1) {
            HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_1, (uint32_t*)ch_dac_config[sch].dac_buffer, DAC_LENGTH, DAC_ALIGN_12B_R);
            Control_Tim_Clk(1, 1);
        } else if (ch == 2) {
            HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_2, (uint32_t*)ch_dac_config[sch].dac_buffer, DAC_LENGTH, DAC_ALIGN_12B_R);
            Control_Tim_Clk(2, 1);
        }
    } else {
        if (ch == 1) {
            Control_Tim_Clk(1, 0);
            HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1);
        } else if (ch == 2) {
            Control_Tim_Clk(2, 0);
            HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_2);
        }
    }
}

void Calc_DAC_Buffer(uint8_t ch) {
    if (ch != 1 && ch != 2) return;
    
    uint8_t sch = Switch_Channel_Input(ch);
    float Vppf = 0.0f;
    if (ch_dac_config[sch].wave_type == SIN_WAVE) {
        Vppf = U12BIT * ch_dac_config[sch].Vpp / DOUBLE_VERF;
        for (int i = 0; i < DAC_LENGTH; i++) {
            ch_dac_config[sch].dac_buffer[i] = (uint16_t)(Vppf * (sinf(PI2 * i / DAC_LENGTH) + 1.0f));
        }
    } else if (ch_dac_config[sch].wave_type == SQU_WAVE) {
        Vppf = U12BIT * ch_dac_config[sch].Vpp / VERF;
        uint16_t high_idx = DAC_LENGTH * ch_dac_config[sch].duty_cycle / 100;
        for (int i = 0; i < DAC_LENGTH; i++) {
            ch_dac_config[sch].dac_buffer[i] = (i < high_idx) ? (uint16_t)Vppf : 0;
        }
    } else if (ch_dac_config[sch].wave_type == TRI_WAVE) {
        Vppf = U12BIT * ch_dac_config[sch].Vpp / VERF;
        uint8_t duty = (ch_dac_config[sch].duty_cycle >= 100) ? 99 : (ch_dac_config[sch].duty_cycle == 0 ? 1 : ch_dac_config[sch].duty_cycle);
        uint16_t RE_idx = DAC_LENGTH * duty / 100;
        float r_scale = Vppf / (float)RE_idx;
        float f_scale = Vppf / (float)(DAC_LENGTH - RE_idx);
        for (int i = 0; i < RE_idx; i++) {
            ch_dac_config[sch].dac_buffer[i] = (uint16_t)(r_scale * i);
        }
        for (int i = RE_idx; i < DAC_LENGTH; i++) {
            ch_dac_config[sch].dac_buffer[i] = (uint16_t)(f_scale * (DAC_LENGTH - i));
        }
    }
    ch_dac_config[sch].status = 0;
}

void Set_DAC_Vpp(uint8_t ch, float Vpp) {
    if (ch != 1 && ch != 2) return;
    uint8_t sch = Switch_Channel_Input(ch);
    if (Vpp > 3.3f) Vpp = 3.3f;
    else if (Vpp < 0.0f) Vpp = 0.0f;
    ch_dac_config[sch].Vpp = Vpp;
    ch_dac_config[sch].status = 1;
}

void Set_DAC_Wave_Type(uint8_t ch, uint8_t w_type) {
    if (ch != 1 && ch != 2) return;
    uint8_t sch = Switch_Channel_Input(ch);
    ch_dac_config[sch].wave_type = w_type;
    ch_dac_config[sch].status = 1;
}

void Set_DAC_Freq(uint8_t ch, float freq) {
    if (ch != 1 && ch != 2) return;
    uint8_t sch = Switch_Channel_Input(ch);
    ch_dac_config[sch].freq = freq;
    Set_Tim_Freq(ch, freq * DAC_LENGTH);
    ch_dac_config[sch].status = 1;
}

void Set_DAC_Duty(uint8_t ch, uint8_t duty) {
    if (ch != 1 && ch != 2) return;
    uint8_t sch = Switch_Channel_Input(ch);
    ch_dac_config[sch].duty_cycle = duty;
    ch_dac_config[sch].status = 1;
}

uint32_t Get_DAC_Freq(uint8_t ch) {
	if (ch != 1 && ch != 2) return 0;
    uint8_t sch = Switch_Channel_Input(ch);
    return ch_dac_config[sch].freq;
}

float Get_DAC_Vpp(uint8_t ch) {
	if (ch != 1 && ch != 2) return 0.0f;
    uint8_t sch = Switch_Channel_Input(ch);
    return ch_dac_config[sch].Vpp;
}

uint8_t Get_DAC_Duty(uint8_t ch) {
	if (ch != 1 && ch != 2) return 0;
    uint8_t sch = Switch_Channel_Input(ch);
    return ch_dac_config[sch].duty_cycle;
}

uint8_t Get_DAC_Wave_Type(uint8_t ch) {
	if (ch != 1 && ch != 2) return 0;
    uint8_t sch = Switch_Channel_Input(ch);
    return ch_dac_config[sch].wave_type;
}

uint8_t Get_DAC_Status(uint8_t ch) {
    if (ch != 1 && ch != 2) return 0;
    uint8_t sch = Switch_Channel_Input(ch);
    return ch_dac_config[sch].status;
}
