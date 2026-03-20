#include "adc_control.h"
#include "window_value.h"
#include "tim_control.h"

#include <stdint.h>
#include <string.h>
#include "arm_math.h"
#include "arm_const_structs.h"

/*
工作流程：
开启通道->采集数据至fft数组->关闭通道->fft计算->调节采样显示频率->开启通道->关闭通道->处理显示数组数据->下一轮采集数据至fft数组...

fft计算流程：如果fft峰值幅度误差较大，则采样频率降低10k，开启通道进行下一次采样，直到误差满足要求或者采集次数达到上限，满足要求则使用双主峰间频率，否则将使用主峰频率
调节采样显示频率流程：如果fft频率小于100khz，则以100倍过采样进行采样，否则以100倍相对采样频率进行采样
*/

#define abs(x) (x) > 0 ? (x) : -(x)

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

typedef enum {
	ADC_OFF = 0,
	ADC_FFT_ORDER,
	ADC_FFT_LOCK,
	ADC_SHOW_ORDER,
	ADC_SHOW_LOCK,
} ADC_STATUS;

// OFF-> (FFT_ORDER -> FFT_LOCK -> FFT_ORDER -> FFT_LOCK ->...) -> SHOW_ORDER -> SHOW_LOCK -> (FFT_ORDER ->...) / ...-> OFF

typedef struct {
	ADC_STATUS status;
	uint32_t fft_wpos;
	uint32_t show_wpos;
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

// =============== 辅助函数 ===============

static uint8_t inline Switch_Channel_Input(uint8_t ch) {
	if (ch == 1 || ch == 2) {
		return (ch - 1);
	}
	return 0;
}

// =============== 初始化函数 ===============

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

// =============== 核心控制函数 ===============

// 凡是启用时都会清除adc数组
void Control_ADC_Enable(uint8_t ch, uint8_t enable) {
	if (ch == 1) {
		memset(adc_buffer[0], 0, sizeof(uint8_t) * ADC_LENGTH);
		if (enable) {
			HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);
			HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_buffer[0], ADC_LENGTH);
			Control_Tim_Clk(1, 1);
		} else {
			Control_Tim_Clk(1, 0);
			HAL_ADC_Stop_DMA(&hadc1);
		}
	} else if (ch == 2) {
		memset(adc_buffer[1], 0, sizeof(uint8_t) * ADC_LENGTH);
		if (enable) {
			HAL_ADCEx_Calibration_Start(&hadc2, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);
			HAL_ADC_Start_DMA(&hadc2, (uint32_t *)adc_buffer[1], ADC_LENGTH);
			Control_Tim_Clk(2, 1);
		} else {
			Control_Tim_Clk(2, 0);
			HAL_ADC_Stop_DMA(&hadc2);	
		}
	}
}

// =============== 计算和查询函数 ===============

uint8_t Calc_Comp_FFT_Ampl(uint8_t ch) {
	if (ch != 1 && ch != 2) return 0;
	
	uint8_t sch = Switch_Channel_Input(ch);

	float mean;
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
		return 0;
	} else {
		ch_fft_config[sch].fft_running_time++;
		if (ch_fft_config[sch].fft_running_time >= SHIFT_MAX_RANK) {
			float sample_freq = (float)Get_Tim_Freq(ch);
			ch_show_config[sch].fft_freq = (float)ch_fft_config[sch].first_max_index * sample_freq / (float)FFT_LENGTH;
			ch_fft_config[sch].fft_running_time = 0;
			return 0;
		}
		return 1;
	}
}

// =============== 采样流程 ===============

// 在flag=1后调用，返回采样类型：0=重试，1=低频过采样，2=高频ETS采样
uint8_t Sample_Until_FFT(uint8_t ch) {
	if (ch != 1 && ch != 2) return 0;
	
	uint8_t sch = Switch_Channel_Input(ch);
	uint8_t calc_fail = Calc_Comp_FFT_Ampl(ch);
	if (calc_fail) {
		Set_Tim_Freq(ch, Get_Tim_Freq(ch) - SAMPLE_FREQ_SHIFT);
		ch_running_config[sch].status = ADC_FFT_ORDER;
		Control_ADC_Enable(ch, 1);
		return 0;
	} else {
		Control_ADC_Enable(ch, 0);
		if (ch_show_config[sch].fft_freq < MAX_OVER_SAMPLE_FREQ) {
			Set_Tim_Freq(ch, (uint32_t)(ch_show_config[sch].fft_freq * OVER_SAMPLE_RATE));
			ch_running_config[sch].status = ADC_SHOW_ORDER;
			Control_ADC_Enable(ch, 1);
			return 1;
		} else {
			Set_Tim_Freq(ch, (uint32_t)(ch_show_config[sch].fft_freq * (1.0f - 1.0f / ETS_SAMPLE_RATE)));
			ch_running_config[sch].status = ADC_SHOW_ORDER;
			Control_ADC_Enable(ch, 1);
			return 2;
		}
	}
}

// =============== 内部计算函数 ===============

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

void Process_Show_Buffer(uint32_t ch, uint8_t * array, uint32_t length) {
	if (ch != 1 && ch != 2) return;
	
	uint8_t sch = Switch_Channel_Input(ch);
	uint32_t rising_pos = Calc_Rising_Edge_Pos(ch);
	length = (rising_pos + length <= SHOW_LENGTH) ? length : (SHOW_LENGTH - rising_pos);
	memcpy(array, &show_buffer[sch][rising_pos], sizeof(uint8_t) * length);
	memset(show_buffer[sch], 0, sizeof(uint8_t) * SHOW_LENGTH);
}

// =============== DMA 回调处理 ===============

void Callback_Control(uint8_t ch, uint32_t adc_pos, uint32_t length) {
	if (ch != 1 && ch != 2) return;
	
	uint8_t sch = Switch_Channel_Input(ch);
	if (ch_running_config[sch].status == ADC_FFT_ORDER) {
		uint32_t fft_wpos = ch_running_config[sch].fft_wpos;
		for (int i = 0; i < length; i++) {
			fft_buffer[sch][fft_wpos] = (float)adc_buffer[sch][(adc_pos + i) % ADC_LENGTH];
			fft_wpos++;
		}
		if (fft_wpos >= FFT_LENGTH) {
			Control_ADC_Enable(ch, 0);
			ch_running_config[sch].status = ADC_FFT_LOCK;
			ch_running_config[sch].fft_wpos = 0;
			over_flag[sch] = 1;
		} else {
			ch_running_config[sch].fft_wpos = fft_wpos;
		}
	} else if (ch_running_config[sch].status == ADC_SHOW_ORDER) {
		uint32_t show_wpos = ch_running_config[sch].show_wpos;
		for (int i = 0; i < length; i++) {
			show_buffer[sch][show_wpos] = adc_buffer[sch][(adc_pos + i) % ADC_LENGTH];
			show_wpos++;
		}
		if (show_wpos >= SHOW_LENGTH) {
			Control_ADC_Enable(ch, 0);
			ch_running_config[sch].status = ADC_SHOW_LOCK;
			ch_running_config[sch].show_wpos = 0;
			over_flag[sch] = 1;
		} else {
			ch_running_config[sch].show_wpos = show_wpos;
		}
	}
}

// =============== 中断回调函数 ===============

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

// =============== 状态和参数查询函数 ===============

uint8_t Get_ADC_Running_Status(uint8_t ch) {
	if (ch == 1 || ch == 2) {
		uint8_t sch = Switch_Channel_Input(ch);
		return ch_running_config[sch].status;
	}
	return 0;
}

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
