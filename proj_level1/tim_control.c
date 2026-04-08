/**
 * @file tim_control.c
 * @brief Timer and clock control module for ADC/DAC sampling
 * 
 * Manages dual-channel external clock generation using SI5351:
 * - Channel 1 (CLK0): Controls ADC sampling or square wave frequency
 * - Channel 2 (CLK2): Controls ADC sampling or square wave frequency
 * Also manages TIM2 and TIM8 for synchronization.
 */

#include "tim_control.h"
#include "bsp_si5351.h"
#include "tim.h"
#include "string.h"

extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim8;

/* =========================== Constants and Structures =========================== */

#define EXTIM_CH1_ARR 2u        // Timer prescaler/divider for channel 1
#define EXTIM_CH2_ARR 2u        // Timer prescaler/divider for channel 2
#define SI5351_4MA 1u           // SI5351 output drive strength (4mA)

/** Configuration state for each timer channel */
typedef struct {
	uint32_t freq;              // Current frequency setting (Hz)
	uint8_t status;             // Timer enable/disable status
} Tim_Config_Struct;

static Tim_Config_Struct ch1_config, ch2_config;

/* =========================== Initialization =========================== */

/**
 * Initialize timer and clock system
 * - Reset both channel configurations
 * - Set default frequency (1kHz)
 * - Initialize SI5351 oscillator and configure output clocks
 */
void Tim_Control_Init(void) {
	memset(&ch1_config, 0, sizeof(Tim_Config_Struct));
	memset(&ch2_config, 0, sizeof(Tim_Config_Struct));
	
	// Set default frequencies
	ch1_config.freq = 1000;
	ch2_config.freq = 1000;
	
	// Initialize SI5351 programmable oscillator
	BSP_SI5351_Init();
	BSP_SI5351_SetupCLK0(1000, SI5351_4MA);
	BSP_SI5351_SetupCLK2(1000, SI5351_4MA);
}


/* =========================== Frequency Management =========================== */

/**
 * Get current frequency setting for a channel (Hz)
 */
uint32_t Get_Tim_Freq(uint8_t ch) {
	if (ch == 1) return ch1_config.freq;
	else if (ch == 2) return ch2_config.freq;
	return 0;
}

/**
 * Set output frequency for a channel
 * Configures SI5351 output clock to desired frequency
 */ 
void Set_Tim_Freq(uint8_t ch, uint32_t freq) {
	if (ch == 1) {
		ch1_config.freq = freq;
		// Apply prescaler and configure SI5351 CLK0
		BSP_SI5351_SetupCLK0(ch1_config.freq * EXTIM_CH1_ARR, SI5351_4MA);
	} else if (ch == 2) {
		ch2_config.freq = freq;
		// Apply prescaler and configure SI5351 CLK2
		BSP_SI5351_SetupCLK2(ch2_config.freq * EXTIM_CH2_ARR, SI5351_4MA);
	}
}

/* =========================== Timer Control =========================== */

/**
 * Enable or disable timer and clock output for a channel
 * When enabled: starts SI5351 output and corresponding timer (TIM2 or TIM8)
 * When disabled: stops both SI5351 output and timer
 */
void Control_Tim_Clk(uint8_t ch, uint8_t enable) {
	if (ch == 1) {
		ch1_config.status = enable;
		if (enable) {
			BSP_SI5351_EnableControl(0, 1);  // Enable SI5351 CLK0
			HAL_TIM_Base_Start(&htim2);     // Start TIM2 for sync
		} else {
			BSP_SI5351_EnableControl(0, 0);  // Disable SI5351 CLK0
			HAL_TIM_Base_Stop(&htim2);      // Stop TIM2
		}
	} else if (ch == 2) {
		ch2_config.status = enable;
		if (enable) {
			BSP_SI5351_EnableControl(2, 1);  // Enable SI5351 CLK2
			HAL_TIM_Base_Start(&htim8);     // Start TIM8 for sync
		} else {
			BSP_SI5351_EnableControl(2, 0);  // Disable SI5351 CLK2
			HAL_TIM_Base_Stop(&htim8);      // Stop TIM8
		}
	}
}
