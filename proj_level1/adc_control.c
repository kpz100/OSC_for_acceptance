/**
 * @file adc_control.c
 * @brief Multi-channel ADC acquisition and FFT analysis module
 * 
 * Handles dual-channel ADC data collection with DMA transfers, FFT frequency analysis,
 * and waveform display processing. Supports automatic frequency detection and
 * flexible sampling rate adjustment.
 */

#include "adc_control.h"
#include "tim_control.h"
#include "window_value.h"

#include <stdio.h>
#include <string.h>
#include "arm_math.h"
#include "arm_const_structs.h"

/* =========================== Data Structures =========================== */

typedef struct {
	uint32_t fft_wpos;
	uint32_t show_wpos;
	uint8_t fft_flag;
	uint8_t show_flag;
	// rw_target: Controls which buffer (FFT or SHOW) the ADC callback writes to.
	// The DMA callback uses this to alternate between collecting FFT data and display data.
	uint8_t rw_target;
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


/* =========================== Static Buffers and State =========================== */

// ADC raw data buffer (circular 1k samples per channel)
static uint8_t adc_buffer[ADC_CHANNEL_NUM][ADC_LENGTH] 
    __attribute__((section(".bss.MPU_REGION_2"))) __ALIGNED(32);

// Waveform display buffer (2k samples per channel)
static uint8_t show_buffer[ADC_CHANNEL_NUM][SHOW_LENGTH] 
    __attribute__((section(".bss.MPU_REGION_2"))) __ALIGNED(32);

// FFT input buffer (4k floats per channel)
static float fft_buffer[ADC_CHANNEL_NUM][FFT_LENGTH] 
    __attribute__((section(".bss.MPU_REGION_1"))) __ALIGNED(32);

// FFT magnitude output buffer (2k floats per channel)
static float mag_buffer[ADC_CHANNEL_NUM][MAG_LENGTH] 
    __attribute__((section(".bss.MPU_REGION_1"))) __ALIGNED(32);

// Configuration and state arrays
static FFT_Max_Struct ch_fft_config[ADC_CHANNEL_NUM];
static ADC_Show_Struct ch_show_config[ADC_CHANNEL_NUM];
static ADC_Running_Struct ch_running_config[ADC_CHANNEL_NUM];

// FFT computation handler
static arm_rfft_fast_instance_f32 fft_handler;
static const float * the_window = NULL;

/* =========================== Utility Functions =========================== */

/** Convert external channel number (1-2) to internal index (0-1) */
static inline uint8_t Switch_Channel_Input(uint8_t ch) {
	return (ch == 1 || ch == 2) ? (ch - 1) : 0;
}

/** Validate channel number and return internal index, or -1 if invalid */
static inline int8_t Get_And_Validate_Channel(uint8_t ch) {
	if (ch != 1 && ch != 2) return -1;
	return (int8_t)(ch - 1);
}

/** Get ADC handler for given channel */
static inline ADC_HandleTypeDef* Get_ADC_Handler(uint8_t ch) {
	return (ch == 1) ? &hadc1 : &hadc2;
}


/** Update sampling frequency for a channel */
static void Inner_Set_Freq(uint8_t ch, uint32_t freq) {
	int8_t sch = Get_And_Validate_Channel(ch);
	if (sch >= 0) {
		Set_Tim_Freq(ch, freq);
		ch_show_config[sch].sample_freq = (float)freq;
	}
}

/** Stop ADC and timer, clear buffers */
static void Stop_ADC_Channel(uint8_t ch) {
	int8_t sch = Get_And_Validate_Channel(ch);
	if (sch < 0) return;
	
	Control_Tim_Clk(ch, 0);
	HAL_ADC_Stop_DMA(Get_ADC_Handler(ch));
	memset(adc_buffer[sch], 0, sizeof(uint8_t) * ADC_LENGTH);
}

/** Initialize and start ADC/timer for a channel */
static void Start_ADC_Channel(uint8_t ch) {
	int8_t sch = Get_And_Validate_Channel(ch);
	if (sch < 0) return;
	
	ADC_HandleTypeDef* adc = Get_ADC_Handler(ch);
	ch_running_config[sch].rw_target = RW_TARGET_FFT;
	
	HAL_ADCEx_Calibration_Start(adc, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);
	HAL_ADC_Start_DMA(adc, (uint32_t *)adc_buffer[sch], ADC_LENGTH);
	Control_Tim_Clk(ch, 1);
}

/* =========================== Initialization =========================== */

/**
 * Initialize ADC, buffers, and FFT engine
 * - Clear all buffers
 * - Set up FFT handler with Hamming window
 * - Configure timers and sampling frequencies
 */
void ADC_FFT_Init(void) {
	// Clear all buffers and state
	memset(adc_buffer, 0, sizeof(uint8_t) * ADC_CHANNEL_NUM * ADC_LENGTH);
	memset(show_buffer, 0, sizeof(uint8_t) * ADC_CHANNEL_NUM * SHOW_LENGTH);
	memset(fft_buffer, 0, sizeof(float) * ADC_CHANNEL_NUM * FFT_LENGTH);
	memset(mag_buffer, 0, sizeof(float) * ADC_CHANNEL_NUM * MAG_LENGTH);
	memset(ch_fft_config, 0, sizeof(FFT_Max_Struct) * ADC_CHANNEL_NUM);
	memset(ch_show_config, 0, sizeof(ADC_Show_Struct) * ADC_CHANNEL_NUM);
	memset(ch_running_config, 0, sizeof(ADC_Running_Struct) * ADC_CHANNEL_NUM);

	// Initialize FFT engine and windowing function
	the_window = get_window(FFT_LENGTH);
	arm_rfft_fast_init_f32(&fft_handler, FFT_LENGTH);

	// Configure both channels
	for (uint8_t ch = 1; ch <= 2; ch++) {
		Stop_ADC_Channel(ch);
		Inner_Set_Freq(ch, ORIGINAL_SAMPLE_FREQ);
	}
}


/* =========================== FFT and Frequency Analysis =========================== */

/**
 * Calculate FFT and find dominant frequency
 * Returns 1 when frequency is confidently detected (peak error < threshold or max attempts reached)
 * Returns 0 while still converging (needs to adjust sampling frequency and retry)
 */
__attribute__((section(".text.fast_code"))) uint8_t Calc_Comp_FFT_Ampl(uint8_t ch) {
	int8_t sch = Get_And_Validate_Channel(ch);
	if (sch < 0) return 0;

	// Remove DC offset
	float mean = 0.0f;
	arm_mean_f32(fft_buffer[sch], FFT_LENGTH, &mean);
	arm_offset_f32(fft_buffer[sch], -mean, fft_buffer[sch], FFT_LENGTH);
	
	// Apply Hamming window to reduce spectral leakage
	if (the_window) {
		arm_mult_f32(fft_buffer[sch], the_window, fft_buffer[sch], FFT_LENGTH);
	}

	// Compute real FFT and magnitude spectrum
	arm_rfft_fast_f32(&fft_handler, fft_buffer[sch], fft_buffer[sch], 0);
	arm_cmplx_mag_f32(fft_buffer[sch], mag_buffer[sch], MAG_LENGTH);
	
	// Find the first (highest) peak in frequency spectrum
	arm_max_f32(mag_buffer[sch], MAG_LENGTH, &ch_fft_config[sch].first_mag_max, &ch_fft_config[sch].first_max_index);

	// Get the second peak (next to the first peak for error checking)
	ch_fft_config[sch].second_max_index = ch_fft_config[sch].first_max_index + 1;
	ch_fft_config[sch].second_mag_max = mag_buffer[sch][ch_fft_config[sch].second_max_index];
	
	// Calculate first round frequency for warning check
	float sample_freq = (float)Get_Tim_Freq(ch);
	float first_round_freq = ((float)ch_fft_config[sch].first_max_index + 0.5f) * sample_freq / (float)FFT_LENGTH;
	
	// Check if frequency exceeds warning threshold - output immediately if so
	if (first_round_freq > WARNNING_FFT_FREQ) {
		ch_show_config[sch].fft_freq = first_round_freq;
		ch_fft_config[sch].fft_running_time = 0;
		
		float fft_peak_error = (ch_fft_config[sch].first_mag_max - ch_fft_config[sch].second_mag_max) / ch_fft_config[sch].first_mag_max;	
		
		return 1;
	}
	
	// Check if frequency detection is confident (peaks are well separated)
	float fft_peak_error = (ch_fft_config[sch].first_mag_max - ch_fft_config[sch].second_mag_max) / ch_fft_config[sch].first_mag_max;
	if (fft_peak_error < MAX_FFT_AMP_ERROR) {
		// Confident frequency detection - use interpolated peak position
		ch_show_config[sch].fft_freq = first_round_freq;
		ch_fft_config[sch].fft_running_time = 0;

		return 1;
	} else {
		// Not confident yet - need to adjust sampling rate and retry
		ch_fft_config[sch].fft_running_time++;

		// After max attempts, settle on integer bin frequency
		if (ch_fft_config[sch].fft_running_time >= SHIFT_MAX_RANK) {
			float sample_freq = (float)Get_Tim_Freq(ch);
			ch_show_config[sch].fft_freq = (float)ch_fft_config[sch].first_max_index * sample_freq / (float)FFT_LENGTH;
			ch_fft_config[sch].fft_running_time = 0;

			return 1;
		}
		return 0;
	}
}

/* =========================== ADC Control =========================== */

/**
 * Enable or disable ADC channel acquisition
 */
void Control_ADC_Enable(uint8_t ch, uint8_t enable) {
	int8_t sch = Get_And_Validate_Channel(ch);
	if (sch < 0) return;

	if (enable) {
		memset(adc_buffer[sch], 0, sizeof(uint8_t) * ADC_LENGTH);
		Start_ADC_Channel(ch);
	} else {
		ch_running_config[sch].rw_target = RW_TARGET_NONE;
		Stop_ADC_Channel(ch);
	}
}

/**
 * Adjust sampling frequency and restart ADC capture
 * Used during FFT frequency search to fine-tune the sampling rate
 */
void Set_Sample_Freq(uint8_t ch, uint32_t freq) {
	int8_t sch = Get_And_Validate_Channel(ch);
	if (sch < 0) return;
	
	Stop_ADC_Channel(ch);
	Inner_Set_Freq(ch, freq);
	Start_ADC_Channel(ch);
}

/* =========================== Waveform Display Processing =========================== */

/**
 * Calculate peak-to-peak voltage from display buffer (8-bit representation)
 * Scans the full SHOW_LENGTH buffer and returns 1 if Vpp is significant (> 5 LSBs)
 */
uint8_t Calc_Vpp8(uint8_t ch) {
	int8_t sch = Get_And_Validate_Channel(ch);
	if (sch < 0) return 0;
	
	uint8_t max8 = 0, min8 = 255;
	for (int i = 0; i < SHOW_LENGTH; i++) {
		uint8_t temp = show_buffer[sch][i];
		if (temp > max8) max8 = temp;
		if (temp < min8) min8 = temp;
	}
	ch_show_config[sch].Vpp_8 = max8 - min8;
	return (ch_show_config[sch].Vpp_8 > 5);
}

/**
 * Find the rising edge position in the display buffer
 * Uses Vpp_8/2 as threshold to detect signal transition
 */
uint32_t Calc_Rising_Edge_Pos(uint8_t ch) {
	int8_t sch = Get_And_Validate_Channel(ch);
	if (sch < 0) return 0;
	
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

/**
 * Extract waveform starting from rising edge
 * Copies data from show_buffer to external array
 */
void Process_Show_Buffer(uint32_t ch, uint8_t * array, uint32_t length, uint8_t clear_after) {
	int8_t sch = Get_And_Validate_Channel(ch);
	if (sch < 0) return;
	
	uint32_t rising_pos = Calc_Rising_Edge_Pos(ch);
	length = (rising_pos + length <= SHOW_LENGTH) ? length : (SHOW_LENGTH - rising_pos);
	memcpy(array, &show_buffer[sch][rising_pos], sizeof(uint8_t) * length);
	if (clear_after) {
		memset(show_buffer[sch], 0, sizeof(uint8_t) * SHOW_LENGTH);
	}
}

/**
 * Get pointer to raw show buffer for a channel
 */
uint8_t* Get_Show_Buffer(uint8_t ch) {
	int8_t sch = Get_And_Validate_Channel(ch);
	return (sch >= 0) ? show_buffer[sch] : NULL;
}

/**
 * Calculate available display length from a read position
 */
uint32_t Get_Available_Show_Length(uint32_t read_pos) {
	uint32_t available_length = SHOW_LENGTH - read_pos;
	return (available_length > 0) ? available_length : 0;
}

/* =========================== DMA Callbacks and Data Transfer =========================== */

/**
 * Core callback function called by HAL ADC DMA handlers
 * Routes raw ADC samples to either FFT buffer or display buffer based on rw_target
 */
void Callback_Control(uint8_t ch, uint32_t adc_wpos, uint32_t length) {
	int8_t sch = Get_And_Validate_Channel(ch);
	if (sch < 0) return;
	
	// Transfer data to FFT buffer if target is FFT
	if (ch_running_config[sch].rw_target == RW_TARGET_FFT && ch_running_config[sch].fft_wpos < FFT_LENGTH) {
		uint32_t fft_wpos = ch_running_config[sch].fft_wpos;
		for (int i = 0; i < length; i++) {
			fft_buffer[sch][fft_wpos] = (float)adc_buffer[sch][(adc_wpos + i) % ADC_LENGTH];
			fft_wpos++;
		}
		ch_running_config[sch].fft_wpos = fft_wpos;

		// FFT buffer full - signal sampling complete
		if (fft_wpos >= FFT_LENGTH) {	
			ch_running_config[sch].rw_target = RW_TARGET_NONE;
			ch_running_config[sch].fft_wpos = 0;
			ch_running_config[sch].fft_flag = 1;
		}
	} 
	// Transfer data to display buffer if target is SHOW
	else if (ch_running_config[sch].rw_target == RW_TARGET_SHOW && ch_running_config[sch].show_wpos < SHOW_LENGTH) {
		uint32_t show_wpos = ch_running_config[sch].show_wpos;
		for (int i = 0; i < length; i++) {
			show_buffer[sch][show_wpos] = adc_buffer[sch][(adc_wpos + i) % ADC_LENGTH];
			show_wpos++;
		}
		ch_running_config[sch].show_wpos = show_wpos;

		// Display buffer full - signal display ready
		if (show_wpos >= SHOW_LENGTH) {
			ch_running_config[sch].rw_target = RW_TARGET_NONE;
			ch_running_config[sch].show_wpos = 0;
			ch_running_config[sch].show_flag = 1;
		}
	}
}


/** ADC1 DMA half-complete callback (first half of circular buffer) */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc) {
	if (hadc == &hadc1) {
		Callback_Control(1, 0, HALF_ADC_LENGTH);
	}
	else if (hadc == &hadc2) {
		Callback_Control(2, 0, HALF_ADC_LENGTH);
	}
}

/** ADC DMA transfer complete callback (second half of circular buffer) */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
	if (hadc == &hadc1) {
		Callback_Control(1, HALF_ADC_LENGTH, HALF_ADC_LENGTH);
	}
	else if (hadc == &hadc2) {
		Callback_Control(2, HALF_ADC_LENGTH, HALF_ADC_LENGTH);
	}
}


/* =========================== Data Query Interface =========================== */

/**
 * Get peak-to-peak voltage (8-bit) for a channel
 */
uint8_t Get_Vpp_8(uint8_t ch) {
	int8_t sch = Get_And_Validate_Channel(ch);
	return (sch >= 0) ? ch_show_config[sch].Vpp_8 : 0;
}

/**
 * Get detected fundamental frequency (Hz)
 */
float Get_FFT_Freq(uint8_t ch) {
	int8_t sch = Get_And_Validate_Channel(ch);
	return (sch >= 0) ? ch_show_config[sch].fft_freq : 0.0f;
}

/**
 * Get current sampling frequency (Hz)
 */
float Get_Sample_Freq(uint8_t ch) {
	int8_t sch = Get_And_Validate_Channel(ch);
	return (sch >= 0) ? ch_show_config[sch].sample_freq : 0.0f;
}


/**
 * Get flag status for a channel (FFT_FLAG_TYPE or SHOW_FLAG_TYPE)
 * 1 = data ready, 0 = not ready
 */
uint8_t Get_ADC_Flag(uint8_t ch, uint8_t flag_type) {
	int8_t sch = Get_And_Validate_Channel(ch);
	if (sch < 0) return 0;
	
	if (flag_type == FFT_FLAG_TYPE) {
		return ch_running_config[sch].fft_flag;
	} else if (flag_type == SHOW_FLAG_TYPE) {
		return ch_running_config[sch].show_flag;
	}
	return 0;
}


/**
 * Set the next data routing target (RW_TARGET_FFT or RW_TARGET_SHOW)
 * Controls whether DMA callback routes data to FFT or display buffer
 */
void Set_Next_Target_Type(uint8_t ch, uint8_t target_type) {
	int8_t sch = Get_And_Validate_Channel(ch);
	if (sch < 0) return;
	
	if (target_type == RW_TARGET_FFT || target_type == RW_TARGET_SHOW) {
		ch_running_config[sch].rw_target = target_type;
	} else {
		ch_running_config[sch].rw_target = RW_TARGET_NONE;
	}
}


/**
 * Clear a flag for a channel after processing
 * Allows DMA callback to signal the next data ready event
 */
void Clear_ADC_Flag(uint8_t ch, uint8_t flag_type) {
	int8_t sch = Get_And_Validate_Channel(ch);
	if (sch < 0) return;
	
	if (flag_type == FFT_FLAG_TYPE) {
		ch_running_config[sch].fft_flag = 0;
	} else if (flag_type == SHOW_FLAG_TYPE) {
		ch_running_config[sch].show_flag = 0;
	}
}
