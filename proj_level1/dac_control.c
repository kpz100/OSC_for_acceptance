/**
 * @file dac_control.c
 * @brief Dual-channel DAC waveform generation module
 * 
 * Supports sine, square, and triangle waveform generation with configurable:
 * - Frequency (Hz)
 * - Peak-to-peak voltage (0-3.3V)
 * - Duty cycle (for square/triangle waves)
 * Uses pre-computed lookup tables and DMA for continuous output.
 */

#include "dac_control.h"
#include "dac.h"
#include "tim_control.h"
#include <string.h>
#include <math.h>

/* =========================== Enumerations and Structures =========================== */

//typedef enum { 
//    SIN_WAVE = 0,       // Sine wave waveform
//    SQU_WAVE,           // Square wave waveform
//    TRI_WAVE            // Triangle wave waveform
//} Wave_Type;

/** Configuration state for each DAC channel */
typedef struct {
    uint16_t dac_buffer[DAC_LENGTH];    // Pre-computed waveform lookup table
    uint8_t wave_type;                // Current waveform type
    uint8_t duty_cycle;                 // Duty cycle percentage (1-99%)
    uint32_t freq;                      // Output frequency in Hz
    float Vpp;                          // Peak-to-peak voltage (0.0-3.3V)
    uint8_t status;                     // Buffer update flag (1=needs recalc, 0=current)
} DAC_Struct;

/* =========================== Static Data =========================== */

static DAC_Struct ch_dac_config[DAC_CHANNEL_NUM] 
    __attribute__((section(".bss.MPU_REGION_2"))) __ALIGNED(32);

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

/** Get DAC channel identifier for HAL library */
static inline uint32_t Get_DAC_Channel(uint8_t ch) {
	return (ch == 1) ? DAC_CHANNEL_1 : DAC_CHANNEL_2;
}


/* =========================== Initialization and Control =========================== */

/**
 * Initialize DAC module
 * - Clear all channel configurations
 * - Set default parameters (1kHz, 3.0Vpp, 50% duty)
 * - Stop both DAC channels
 */
void DAC_Output_Init(void) {
	memset(ch_dac_config, 0, sizeof(DAC_Struct) * DAC_CHANNEL_NUM);
	
	// Configure default parameters for both channels
	for (int i = 0; i < DAC_CHANNEL_NUM; i++) {
		ch_dac_config[i].duty_cycle = 50;
		ch_dac_config[i].freq = 1000;
		ch_dac_config[i].Vpp = 3.0f;
	}
	
	// Ensure both channels are stopped
	for (uint8_t ch = 1; ch <= 2; ch++) {
		Control_Tim_Clk(ch, 0);
		HAL_DAC_Stop_DMA(&hdac1, Get_DAC_Channel(ch));
	}
}

/**
 * Enable or disable DAC output on a channel
 * When enabling: recalculates waveform buffer and starts DMA + timer
 * When disabling: stops timer and DMA transfer
 */
void Control_DAC_Enable(uint8_t ch, uint8_t enable) {
	int8_t sch = Get_And_Validate_Channel(ch);
	if (sch < 0) return;
	
	if (enable) {
		// Calc and load waveform buffer, then start DMA output
		Calc_DAC_Buffer(ch);
		uint32_t dac_ch = Get_DAC_Channel(ch);
		HAL_DAC_Start_DMA(&hdac1, dac_ch, (uint32_t*)ch_dac_config[sch].dac_buffer, DAC_LENGTH, DAC_ALIGN_12B_R);
		
		// Set timer frequency to generate DAC samples at correct rate
		Set_Tim_Freq(ch, ch_dac_config[sch].freq * DAC_LENGTH);
		Control_Tim_Clk(ch, 1);
	} else {
		// Stop timer and DAC DMA
		Control_Tim_Clk(ch, 0);
		HAL_DAC_Stop_DMA(&hdac1, Get_DAC_Channel(ch));
	}
}


/* =========================== Waveform Generation =========================== */

/**
 * Generate waveform lookup table for current configuration
 * Computes DAC buffer based on wave_type, Vpp, and duty_cycle
 * - SIN_WAVE: Standard sine wave (0.0-Vpp peak)
 * - SQU_WAVE: Square wave with configurable duty cycle
 * - TRI_WAVE: Triangle wave with configurable rise/fall symmetry
 */
void Calc_DAC_Buffer(uint8_t ch) {
	int8_t sch = Get_And_Validate_Channel(ch);
	if (sch < 0) return;
	
	float Vppf = 0.0f;
	
	if (ch_dac_config[sch].wave_type == SIN_WAVE) {
		// Sine wave: amplitude = Vpp/2, offset = Vpp/2
		Vppf = U12BIT * ch_dac_config[sch].Vpp / DOUBLE_VERF;
		for (int i = 0; i < DAC_LENGTH; i++) {
			ch_dac_config[sch].dac_buffer[i] = (uint16_t)(Vppf * (sinf(PI2 * i / DAC_LENGTH) + 1.0f));
		}
	} 
	else if (ch_dac_config[sch].wave_type == SQU_WAVE) {
		// Square wave: toggle between 0 and Vpp at duty_cycle point
		Vppf = U12BIT * ch_dac_config[sch].Vpp / VERF;
		uint16_t high_idx = DAC_LENGTH * ch_dac_config[sch].duty_cycle / 100;
		for (int i = 0; i < DAC_LENGTH; i++) {
			ch_dac_config[sch].dac_buffer[i] = (i < high_idx) ? (uint16_t)Vppf : 0;
		}
	} 
	else if (ch_dac_config[sch].wave_type == TRI_WAVE) {
		// Triangle wave: rise from 0 to Vpp, then fall back to 0
		// Transition point set by duty_cycle
		Vppf = U12BIT * ch_dac_config[sch].Vpp / VERF;
		uint8_t duty = (ch_dac_config[sch].duty_cycle >= 100) ? 99 : 
		               (ch_dac_config[sch].duty_cycle == 0) ? 1 : ch_dac_config[sch].duty_cycle;
		
		uint16_t RE_idx = DAC_LENGTH * duty / 100;
		float r_scale = Vppf / (float)RE_idx;           // Rising slope
		float f_scale = Vppf / (float)(DAC_LENGTH - RE_idx);  // Falling slope
		
		// Rising edge
		for (int i = 0; i < RE_idx; i++) {
			ch_dac_config[sch].dac_buffer[i] = (uint16_t)(r_scale * i);
		}
		// Falling edge
		for (int i = RE_idx; i < DAC_LENGTH; i++) {
			ch_dac_config[sch].dac_buffer[i] = (uint16_t)(f_scale * (DAC_LENGTH - i));
		}
	}
	
	// Clear status flag after buffer update
	ch_dac_config[sch].status = 0;
}


/* =========================== Configuration Setters =========================== */

/**
 * Set peak-to-peak voltage for DAC output
 * Clamps value to valid range [0.0V, 3.3V]
 * Flags buffer for recalculation on next output
 */
void Set_DAC_Vpp(uint8_t ch, float Vpp) {
	int8_t sch = Get_And_Validate_Channel(ch);
	if (sch < 0) return;
	
	// Clamp voltage to valid range
	if (Vpp > 3.3f) Vpp = 3.3f;
	else if (Vpp < 0.0f) Vpp = 0.0f;
	
	ch_dac_config[sch].Vpp = Vpp;
	ch_dac_config[sch].status = 1;  // Flag buffer needs recalculation
}

/**
 * Set waveform type (SIN_WAVE, SQU_WAVE, or TRI_WAVE)
 * Flags buffer for recalculation
 */
void Set_DAC_Wave_Type(uint8_t ch, uint8_t w_type) {
	int8_t sch = Get_And_Validate_Channel(ch);
	if (sch < 0) return;
	
	ch_dac_config[sch].wave_type = w_type;
	ch_dac_config[sch].status = 1;  // Flag buffer needs recalculation
}

/**
 * Set output frequency in Hz
 * Updates timer frequency to maintain DAC sample rate = freq * DAC_LENGTH
 * Flags buffer for recalculation
 */
void Set_DAC_Freq(uint8_t ch, float freq) {
	int8_t sch = Get_And_Validate_Channel(ch);
	if (sch < 0) return;
	
	ch_dac_config[sch].freq = freq;
	Set_Tim_Freq(ch, freq * DAC_LENGTH);
	ch_dac_config[sch].status = 1;  // Flag buffer needs recalculation
}

/**
 * Set duty cycle for square/triangle waves
 * Valid range: 1-99% (prevents edge cases)
 * Flags buffer for recalculation
 */
void Set_DAC_Duty(uint8_t ch, uint8_t duty) {
	int8_t sch = Get_And_Validate_Channel(ch);
	if (sch < 0) return;
	
	ch_dac_config[sch].duty_cycle = duty;
	ch_dac_config[sch].status = 1;  // Flag buffer needs recalculation
}

/* =========================== Configuration Getters =========================== */

/**
 * Get current output frequency (Hz)
 */
uint32_t Get_DAC_Freq(uint8_t ch) {
	int8_t sch = Get_And_Validate_Channel(ch);
	return (sch >= 0) ? ch_dac_config[sch].freq : 0;
}

/**
 * Get current peak-to-peak voltage (V)
 */
float Get_DAC_Vpp(uint8_t ch) {
	int8_t sch = Get_And_Validate_Channel(ch);
	return (sch >= 0) ? ch_dac_config[sch].Vpp : 0.0f;
}

/**
 * Get current duty cycle (%)
 */
uint8_t Get_DAC_Duty(uint8_t ch) {
	int8_t sch = Get_And_Validate_Channel(ch);
	return (sch >= 0) ? ch_dac_config[sch].duty_cycle : 0;
}

/**
 * Get current waveform type (0=SIN, 1=SQU, 2=TRI)
 */
uint8_t Get_DAC_Wave_Type(uint8_t ch) {
	int8_t sch = Get_And_Validate_Channel(ch);
	return (sch >= 0) ? ch_dac_config[sch].wave_type : 0;
}

/**
 * Get buffer update status flag
 * 1 = buffer needs recalculation, 0 = buffer is current
 */
uint8_t Get_DAC_Status(uint8_t ch) {
	int8_t sch = Get_And_Validate_Channel(ch);
	return (sch >= 0) ? ch_dac_config[sch].status : 0;
}

