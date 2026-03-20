#include "tim_control.h"
#include "bsp_si5351.h"
#include "tim.h"
#include "string.h"

extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim8;

#define EXTIM_CH1_ARR 2u
#define EXTIM_CH2_ARR 2u
#define SI5351_4MA 1u

typedef struct {
	uint32_t freq;
	uint8_t status;
} Tim_Config_Struct;

static Tim_Config_Struct ch1_config, ch2_config;

void Tim_Control_Init(void) {
	memset(&ch1_config, 0, sizeof(Tim_Config_Struct));
	memset(&ch2_config, 0, sizeof(Tim_Config_Struct));
	ch1_config.freq = 1000;
	ch2_config.freq = 1000;
	BSP_SI5351_Init();
}

uint32_t Get_Tim_Freq(uint8_t ch) {
	if (ch == 1) return ch1_config.freq;
	else if (ch == 2) return ch2_config.freq;
	return 0;
}

void Set_Tim_Freq(uint8_t ch, uint32_t freq) {
	if (ch == 1) {
		ch1_config.freq = freq;
		BSP_SI5351_SetupCLK0(ch1_config.freq * EXTIM_CH1_ARR, SI5351_4MA);
	} else if (ch == 2) {
		ch2_config.freq = freq;
		BSP_SI5351_SetupCLK2(ch2_config.freq * EXTIM_CH2_ARR, SI5351_4MA);
	}
}

void Control_Tim_Clk(uint8_t ch, uint8_t enable) {
	if (ch == 1) {
		ch1_config.status = enable;
		if (enable) {
			BSP_SI5351_EnableControl(0, 1);
			HAL_TIM_Base_Start(&htim2);
		} else {
			BSP_SI5351_EnableControl(0, 0);
			HAL_TIM_Base_Stop(&htim2);
		}
	} else if (ch == 2) {
		ch2_config.status = enable;
		if (enable) {
			BSP_SI5351_EnableControl(2, 1);
			HAL_TIM_Base_Start(&htim8);
		} else {
			BSP_SI5351_EnableControl(2, 0);
			HAL_TIM_Base_Stop(&htim8);
		}
	}
}
