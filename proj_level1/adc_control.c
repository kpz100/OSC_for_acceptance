#include "adc_control.h"
#include "tim_control.h"
#include "window_value.h"

#include <stdio.h>
#include <string.h>
#include "arm_math.h"
#include "arm_const_structs.h"

typedef struct {
	uint32_t fft_wpos;
	uint32_t show_wpos;
	uint8_t fft_flag;
	uint8_t show_flag;
    uint8_t rw_target; // 0=NONE,1=FFT,2=SHOW // 这个变量的意义在于：当ADC完成一次DMA传输时，回调函数会根据这个变量决定下一步处理哪个目标的数据（FFT或SHOW），并在处理完成后更新这个变量以指示下一次应该处理哪个目标。这样可以实现对FFT和SHOW数据的交替处理，确保两者都能及时更新显示。
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

static uint8_t adc_buffer[ADC_CHANNEL_NUM][ADC_LENGTH] 
    __attribute__((section(".bss.MPU_REGION_2"))) __ALIGNED(32);

static uint8_t show_buffer[ADC_CHANNEL_NUM][SHOW_LENGTH] 
    __attribute__((section(".bss.MPU_REGION_2"))) __ALIGNED(32);

static float fft_buffer[ADC_CHANNEL_NUM][FFT_LENGTH] 
    __attribute__((section(".bss.MPU_REGION_1"))) __ALIGNED(32);

static float mag_buffer[ADC_CHANNEL_NUM][MAG_LENGTH] 
    __attribute__((section(".bss.MPU_REGION_1"))) __ALIGNED(32);

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

static void Inner_Set_Freq(uint8_t ch, uint32_t freq) {
	if (ch == 1 || ch == 2) {
		Set_Tim_Freq(ch, freq);
		ch_show_config[Switch_Channel_Input(ch)].sample_freq = (float)freq;
	}
}

void ADC_FFT_Init(void) {
	memset(adc_buffer, 0, sizeof(uint8_t) * ADC_CHANNEL_NUM * ADC_LENGTH);
	memset(show_buffer, 0, sizeof(uint8_t) * ADC_CHANNEL_NUM * SHOW_LENGTH);
	memset(fft_buffer, 0, sizeof(float) * ADC_CHANNEL_NUM * FFT_LENGTH);
	memset(mag_buffer, 0, sizeof(float) * ADC_CHANNEL_NUM * MAG_LENGTH);
	memset(ch_fft_config, 0, sizeof(FFT_Max_Struct) * ADC_CHANNEL_NUM);
	memset(ch_show_config, 0, sizeof(ADC_Show_Struct) * ADC_CHANNEL_NUM);
	memset(ch_running_config, 0, sizeof(ADC_Running_Struct) * ADC_CHANNEL_NUM);

	the_window = get_window(FFT_LENGTH);
	arm_rfft_fast_init_f32(&fft_handler, FFT_LENGTH);

	Control_Tim_Clk(1, 0);
	Inner_Set_Freq(1, ORIGINAL_SAMPLE_FREQ);
	HAL_ADC_Stop_DMA(&hadc1);

	Control_Tim_Clk(2, 0);
	Inner_Set_Freq(2, ORIGINAL_SAMPLE_FREQ);
	HAL_ADC_Stop_DMA(&hadc2);
}

// 也清空数组
void Control_ADC_Enable(uint8_t ch, uint8_t enable) {
    if (ch != 1 && ch != 2) return;

    uint8_t sch = Switch_Channel_Input(ch);
    ADC_HandleTypeDef* adc_handler = (ch == 1) ? &hadc1 : &hadc2;
    if (enable) {
        memset(adc_buffer[sch], 0, sizeof(uint8_t) * ADC_LENGTH);

        ch_running_config[sch].rw_target = RW_TARGET_FFT;
        HAL_ADCEx_Calibration_Start(adc_handler, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);
		HAL_ADC_Start_DMA(adc_handler, (uint32_t *)adc_buffer[sch], ADC_LENGTH);
		Control_Tim_Clk(ch, 1);

		printf("Enable ADC CH%d with Sample Freq %.2fHz\n", ch, Get_Sample_Freq(ch));
    } else {
		ch_running_config[sch].rw_target = RW_TARGET_NONE;
        Control_Tim_Clk(ch, 0);
        HAL_ADC_Stop_DMA(adc_handler);
    }
}

// ===========================================

// 1说明频率命中频谱两个平行最高峰的中间，0没有完成
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
	// arm_max_f32(&mag_buffer[sch][1], MAG_LENGTH - 1, &ch_fft_config[sch].first_mag_max, &ch_fft_config[sch].first_max_index);
	// ch_fft_config[sch].first_max_index += 1;
	arm_max_f32(mag_buffer[sch], MAG_LENGTH, &ch_fft_config[sch].first_mag_max, &ch_fft_config[sch].first_max_index);

	ch_fft_config[sch].second_max_index = ch_fft_config[sch].first_max_index + 1;
	ch_fft_config[sch].second_mag_max = mag_buffer[sch][ch_fft_config[sch].second_max_index];
	
	float fft_peak_error = (ch_fft_config[sch].first_mag_max - ch_fft_config[sch].second_mag_max) / ch_fft_config[sch].first_mag_max;
    if (fft_peak_error < MAX_FFT_AMP_ERROR) {
        float sample_freq = (float)Get_Tim_Freq(ch);
		ch_show_config[sch].fft_freq = ((float)ch_fft_config[sch].first_max_index + 0.5f) * sample_freq / (float)FFT_LENGTH;
		ch_fft_config[sch].fft_running_time = 0;

		printf("FFT: CH%d Freq=%.2fHz, Peak_Error=%.2f%%, Running_Time=%d\n", ch, ch_show_config[sch].fft_freq, fft_peak_error * 100.0f, ch_fft_config[sch].fft_running_time);

		return 1;
    } else {
        ch_fft_config[sch].fft_running_time++;

		printf("Nowtime Sample Freq=%.2fHz\n", Get_Sample_Freq(ch));

		if (ch_fft_config[sch].fft_running_time >= SHIFT_MAX_RANK) {
			float sample_freq = (float)Get_Tim_Freq(ch);
			ch_show_config[sch].fft_freq = (float)ch_fft_config[sch].first_max_index * sample_freq / (float)FFT_LENGTH;
			ch_fft_config[sch].fft_running_time = 0;

			printf("FFT: CH%d Freq=%.2fHz, Peak_Error=%.2f%%, Running_Time=%d\n", ch, ch_show_config[sch].fft_freq, fft_peak_error * 100.0f, ch_fft_config[sch].fft_running_time);

			return 1;
		}
		return 0;
    }
}

// 注意get_freq
void Set_Sample_Freq(uint8_t ch, uint32_t freq) {
    if (ch != 1 && ch != 2) return; 
	
	uint8_t sch = Switch_Channel_Input(ch);
	ADC_HandleTypeDef* adc_handler = (ch == 1) ? &hadc1 : &hadc2;

    Control_Tim_Clk(ch, 0);
    HAL_ADC_Stop_DMA(adc_handler);
	memset(adc_buffer[sch], 0, sizeof(uint8_t) * ADC_LENGTH);
	
    Inner_Set_Freq(ch, freq);
    
	HAL_ADCEx_Calibration_Start(adc_handler, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);
	HAL_ADC_Start_DMA(adc_handler, (uint32_t *)adc_buffer[sch], ADC_LENGTH);
	Control_Tim_Clk(ch, 1);
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
	uint8_t threshold = ch_show_config[sch].Vpp_8 / 2;
	for (int i = 0; i < SHOW_LENGTH - 1; i++) {
		if (show_buffer[sch][i] <= threshold && show_buffer[sch][i + 1] >= threshold) {
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

uint32_t Get_Available_Show_Length(uint32_t read_pos) {
	uint32_t available_length = SHOW_LENGTH - read_pos;
	return (available_length > 0) ? available_length : 0;
}

// ====================================

// 当采集完adc数组后便会停下
void Callback_Control(uint8_t ch, uint32_t adc_wpos, uint32_t length) {
    if (ch != 1 && ch != 2) return;
	
	uint8_t sch = Switch_Channel_Input(ch);
	if (ch_running_config[sch].rw_target == RW_TARGET_FFT && ch_running_config[sch].fft_wpos < FFT_LENGTH) {
		uint32_t fft_wpos = ch_running_config[sch].fft_wpos;
		for (int i = 0; i < length; i++) {
			fft_buffer[sch][fft_wpos] = (float)adc_buffer[sch][(adc_wpos + i) % ADC_LENGTH];
			fft_wpos++;
		}
		ch_running_config[sch].fft_wpos = fft_wpos;

		if (fft_wpos >= FFT_LENGTH) {
			ch_running_config[sch].rw_target = RW_TARGET_NONE;
			ch_running_config[sch].fft_wpos = 0;
			ch_running_config[sch].fft_flag = 1;
		}
	} else if (ch_running_config[sch].rw_target == RW_TARGET_SHOW && ch_running_config[sch].show_wpos < SHOW_LENGTH) {
		uint32_t show_wpos = ch_running_config[sch].show_wpos;
		for (int i = 0; i < length; i++) {
			show_buffer[sch][show_wpos]	= adc_buffer[sch][(adc_wpos + i) % ADC_LENGTH];
			show_wpos++;
		}
		ch_running_config[sch].show_wpos = show_wpos;

		if (show_wpos >= SHOW_LENGTH) {
			ch_running_config[sch].rw_target = RW_TARGET_NONE;
			ch_running_config[sch].show_wpos = 0;
			ch_running_config[sch].show_flag = 1;
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

uint8_t Get_ADC_Flag(uint8_t ch, uint8_t flag_type) {
	if (ch == 1 || ch == 2) {
		uint8_t sch = Switch_Channel_Input(ch);
		if (flag_type == FFT_FLAG_TYPE) {
			return ch_running_config[sch].fft_flag;
		} else if (flag_type == SHOW_FLAG_TYPE) {
			return ch_running_config[sch].show_flag;
		}
	}
	return 0;
}

void Set_Next_Target_Type(uint8_t ch, uint8_t target_type) {
	if (ch == 1 || ch == 2) {
		uint8_t sch = Switch_Channel_Input(ch);
		if (target_type == RW_TARGET_FFT || target_type == RW_TARGET_SHOW) {
			ch_running_config[sch].rw_target = target_type;
		} else {
			ch_running_config[sch].rw_target = RW_TARGET_NONE;
		}
	}
}

void Clear_ADC_Flag(uint8_t ch, uint8_t flag_type) {
	if (ch == 1 || ch == 2) {
		uint8_t sch = Switch_Channel_Input(ch);
		if (flag_type == FFT_FLAG_TYPE) {
			ch_running_config[sch].fft_flag = 0;
			
		} else if (flag_type == SHOW_FLAG_TYPE) {
			ch_running_config[sch].show_flag = 0;

		}
	}
}
