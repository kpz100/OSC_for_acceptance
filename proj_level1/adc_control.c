#include "adc_control.h"
#include "tim_control.h"
#include "window_value.h"

#include <string.h>
#include "arm_math.h"
#include "arm_const_structs.h"

typedef struct {
	uint32_t fft_wpos;
	uint32_t show_wpos;
    uint8_t running;
    uint8_t rw_target; // 0=NONE,1=FFT,2=SHOW
} ADC_Running_Struct;

typedef struct {
	uint32_t first_max_index;
	uint32_t second_max_index;
	float first_mag_max;
	float second_mag_max;
	uint32_t fft_running_time;
} FFT_Max_Struct;

typedef struct {
	float fft_freq;
	float sample_freq;
	uint8_t Vpp_8;
} ADC_Show_Struct;

static uint8_t adc_buffer[ADC_CHANNEL_NUM][ADC_LENGTH] __attribute__((section(".bss.ARM.__at_0x30000000"))) __ALIGNED(32);
static uint8_t show_buffer[ADC_CHANNEL_NUM][SHOW_LENGTH] __attribute__((section(".bss.ARM.__at_0x30000800"))) __ALIGNED(32);
static float fft_buffer[ADC_CHANNEL_NUM][FFT_LENGTH] 
    __attribute__((section(".bss.ARM.__at_0x24004000"))) __ALIGNED(32);

static float mag_buffer[ADC_CHANNEL_NUM][MAG_LENGTH] 
    __attribute__((section(".bss.ARM.__at_0x24006000"))) __ALIGNED(32);
static volatile uint8_t over_flag[ADC_CHANNEL_NUM] = {0};

static FFT_Max_Struct ch_fft_config[ADC_CHANNEL_NUM];
static ADC_Show_Struct ch_show_config[ADC_CHANNEL_NUM];
static ADC_Running_Struct ch_running_config[ADC_CHANNEL_NUM];

static arm_rfft_fast_instance_f32 fft_handler;
static const float * the_window = NULL;

static uint8_t inline Switch_Channel_Input(uint8_t ch) {
	if (ch == 1 || ch == 2) {
		return (ch - 1);
	}
	return 0;
}

void ADC_FFT_Init(void) {
	memset(adc_buffer, 0, sizeof(uint8_t) * ADC_CHANNEL_NUM * ADC_LENGTH);
	memset(show_buffer, 0, sizeof(uint8_t) * ADC_CHANNEL_NUM * SHOW_LENGTH);
	memset(fft_buffer, 0, sizeof(float) * ADC_CHANNEL_NUM * FFT_LENGTH);
	memset(mag_buffer, 0, sizeof(float) * ADC_CHANNEL_NUM * MAG_LENGTH);
	memset(ch_fft_config, 0, sizeof(FFT_Max_Struct) * ADC_CHANNEL_NUM);
	memset(ch_show_config, 0, sizeof(ADC_Show_Struct) * ADC_CHANNEL_NUM);
	memset(ch_running_config, 0, sizeof(ADC_Running_Struct) * ADC_CHANNEL_NUM);

	over_flag[0] = 0;
	over_flag[1] = 0;
	
	the_window = get_window(FFT_LENGTH);
	arm_rfft_fast_init_f32(&fft_handler, FFT_LENGTH);

	Control_Tim_Clk(1, 0);
	HAL_ADC_Stop_DMA(&hadc1);
	Control_Tim_Clk(2, 0);
	HAL_ADC_Stop_DMA(&hadc2);
}

// 也清空数组
void Control_ADC_Enable(uint8_t ch, uint8_t enable) {
    if (ch != 1 && ch != 2) return;

    uint8_t sch = Switch_Channel_Input(ch);
    ADC_HandleTypeDef* adc_handler = (ch == 1) ? &hadc1 : &hadc2;
    if (enable) {
        memset(adc_buffer[sch], 0, sizeof(uint8_t) * ADC_LENGTH);
        ch_running_config[sch].running = 1;
        HAL_ADCEx_Calibration_Start(adc_handler, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);
		HAL_ADC_Start_DMA(adc_handler, (uint32_t *)adc_buffer[sch], ADC_LENGTH);
		Control_Tim_Clk(ch, 1);
    } else {
        Control_Tim_Clk(ch, 0);
        HAL_ADC_Stop_DMA(adc_handler);
    }
}

// ===========================================

// 1说明完成，0没有完成
uint8_t Calc_Comp_FFT_Ampl(uint8_t ch) {
    if (ch != 1 && ch != 2) return 0;

    uint8_t sch = Switch_Channel_Input(ch);

    float mean = 0.0f;
    arm_mean_f32(fft_buffer[sch], FFT_LENGTH, &mean);
	arm_offset_f32(fft_buffer[sch], -mean, fft_buffer[sch], FFT_LENGTH);
	
	if (the_window) {
		arm_mult_f32(fft_buffer[sch], the_window, fft_buffer[sch], FFT_LENGTH);
	}

	arm_rfft_fast_f32(&fft_handler, fft_buffer[sch], fft_buffer[sch], 0);
	arm_cmplx_mag_f32(fft_buffer[sch], mag_buffer[sch], MAG_LENGTH);
	arm_max_f32(&mag_buffer[sch][1], MAG_LENGTH - 1, &ch_fft_config[sch].first_mag_max, &ch_fft_config[sch].first_max_index);
	ch_fft_config[sch].first_max_index += 1;

	ch_fft_config[sch].second_max_index = ch_fft_config[sch].first_max_index + 1;
	ch_fft_config[sch].second_mag_max = mag_buffer[sch][ch_fft_config[sch].second_max_index];
	
	float fft_peak_error = (ch_fft_config[sch].first_mag_max - ch_fft_config[sch].second_mag_max) / ch_fft_config[sch].first_mag_max;
    if (fft_peak_error < MAX_FFT_AMP_ERROR) {
        float sample_freq = (float)Get_Tim_Freq(ch);
		ch_show_config[sch].fft_freq = ((float)ch_fft_config[sch].first_max_index + 0.5f) * sample_freq / (float)FFT_LENGTH;
		ch_fft_config[sch].fft_running_time = 0;
		return 1;
    } else {
        ch_fft_config[sch].fft_running_time++;
		if (ch_fft_config[sch].fft_running_time >= SHIFT_MAX_RANK) {
			float sample_freq = (float)Get_Tim_Freq(ch);
			ch_show_config[sch].fft_freq = (float)ch_fft_config[sch].first_max_index * sample_freq / (float)FFT_LENGTH;
			ch_fft_config[sch].fft_running_time = 0;
			return 1;
		}
		return 0;
    }
}

// 注意get_freq
void Set_Sample_Freq(uint8_t ch, uint32_t freq) {
    if (ch != 1 && ch != 2) return; 

    Control_ADC_Enable(ch, 0);
    Set_Tim_Freq(ch, freq);
    Control_ADC_Enable(ch, 1);
}

// ======================================================

uint8_t Calc_Vpp8(uint8_t ch) {
	if (ch != 1 && ch != 2) return 0;
	
	uint8_t sch = Switch_Channel_Input(ch);
	uint8_t max8 = 0, min8 = 255;
	uint8_t temp = 0;
	for (int i = 0; i < SHOW_LENGTH; i++) {
		temp = show_buffer[sch][i];
		if (temp > max8) max8 = temp;
		if (temp < min8) min8 = temp;
	}
	ch_show_config[sch].Vpp_8 = max8 - min8;
	return (ch_show_config[sch].Vpp_8 > 5);
}

uint32_t Calc_Rising_Edge_Pos(uint8_t ch) {
	if (ch != 1 && ch != 2) return 0;
	
	uint8_t sch = Switch_Channel_Input(ch);
	uint32_t rising_pos = 0;
	for (int i = 0; i < SHOW_LENGTH - 1; i++) {
		if (show_buffer[sch][i] <= ch_show_config[sch].Vpp_8 / 2 && show_buffer[sch][i + 1] >= ch_show_config[sch].Vpp_8 / 2) {
			rising_pos = i;
			break;
		}
	}
	return rising_pos;
}

void Process_Show_Buffer(uint32_t ch, uint8_t * array, uint32_t length, uint8_t clear_after) {
	if (ch != 1 && ch != 2) return;
	
	uint8_t sch = Switch_Channel_Input(ch);
	uint32_t rising_pos = Calc_Rising_Edge_Pos(ch);
	length = (rising_pos + length <= SHOW_LENGTH) ? length : (SHOW_LENGTH - rising_pos);
	memcpy(array, &show_buffer[sch][rising_pos], sizeof(uint8_t) * length);
	if (clear_after) {
		memset(show_buffer[sch], 0, sizeof(uint8_t) * SHOW_LENGTH);
	}
}

uint8_t* Get_Show_Buffer(uint8_t ch) {
	if (ch != 1 && ch != 2) return NULL;
	
	uint8_t sch = Switch_Channel_Input(ch);
	return show_buffer[sch];
}

// ====================================

void Callback_Control(uint8_t ch, uint32_t adc_wpos, uint32_t length) {
    if (ch != 1 && ch != 2) return;
	
	uint8_t sch = Switch_Channel_Input(ch);
	if (ch_running_config[sch].rw_target == RW_TARGET_FFT && ch_running_config[sch].fft_wpos < FFT_LENGTH) {
		uint32_t fft_wpos = ch_running_config[sch].fft_wpos;
		for (int i = 0; i < length; i++) {
			fft_buffer[sch][fft_wpos] = (float)adc_buffer[sch][(adc_wpos + i) % ADC_LENGTH];
			fft_wpos++;
		}
		if (fft_wpos >= FFT_LENGTH) {
			ch_running_config[sch].rw_target = RW_TARGET_NONE;
			ch_running_config[sch].fft_wpos = 0;
			over_flag[sch] = 1;
		}
	} else if (ch_running_config[sch].rw_target == RW_TARGET_SHOW && ch_running_config[sch].show_wpos < SHOW_LENGTH) {
		uint32_t show_wpos = ch_running_config[sch].show_wpos;
		for (int i = 0; i < length; i++) {
			show_buffer[sch][show_wpos]	= adc_buffer[sch][(adc_wpos + i) % ADC_LENGTH];
			show_wpos++;
		}
		if (show_wpos >= SHOW_LENGTH) {
			ch_running_config[sch].rw_target = RW_TARGET_NONE;
			ch_running_config[sch].show_wpos = 0;
			over_flag[sch] = 1;
		}
	}
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc) {
    if (hadc == &hadc1) {
		Callback_Control(1, 0, HALF_ADC_LENGTH);
	}
    else if (hadc == &hadc2) {
		Callback_Control(2, 0, HALF_ADC_LENGTH);
	}
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
	if (hadc == &hadc1) {
		Callback_Control(1, HALF_ADC_LENGTH, HALF_ADC_LENGTH);
	}
	else if (hadc == &hadc2) {
		Callback_Control(2, HALF_ADC_LENGTH, HALF_ADC_LENGTH);
	}
}

// =======================================

uint8_t Get_Vpp_8(uint8_t ch) {
	if (ch == 1 || ch == 2) {
		uint8_t sch = Switch_Channel_Input(ch);
		return ch_show_config[sch].Vpp_8;
	}
	return 0;
}

float Get_FFT_Freq(uint8_t ch) {
	if (ch == 1 || ch == 2) {
		uint8_t sch = Switch_Channel_Input(ch);
		return ch_show_config[sch].fft_freq;
	}
	return 0.0f;
}

float Get_Sample_Freq(uint8_t ch) {
	if (ch == 1 || ch == 2) {
		uint8_t sch = Switch_Channel_Input(ch);
		return ch_show_config[sch].sample_freq;
	}
	return 0.0f;
}

uint8_t Get_ADC_Flag(uint8_t ch) {
	if (ch == 1 || ch == 2) {
		uint8_t sch = Switch_Channel_Input(ch);
		return over_flag[sch];
	}
	return 0;
}

void Clear_ADC_Flag(uint8_t ch, uint8_t next_target_type) {
	if (ch == 1 || ch == 2) {
		uint8_t sch = Switch_Channel_Input(ch);
		over_flag[sch] = 0;
		if (next_target_type == RW_TARGET_FFT) {
			ch_running_config[sch].fft_wpos = 0;
			ch_running_config[sch].rw_target = RW_TARGET_FFT;
		} else if (next_target_type == RW_TARGET_SHOW) {
			ch_running_config[sch].show_wpos = 0;
			ch_running_config[sch].rw_target = RW_TARGET_SHOW;
		} else {
			ch_running_config[sch].rw_target = RW_TARGET_NONE;
		}
	}
}
