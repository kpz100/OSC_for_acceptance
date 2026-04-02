#include "new_test1.h"
#include "bsp_dwt.h"
#include "bsp_sdram.h"
#include "bsp_lcd_single.h"
#include "bsp_touch.h"
#include "bsp_si5351.h"
#include "tim.h"
#include "ascii_font.h"
#include <stdio.h>
#include <string.h>

#include "ui_rampool.h"
#include "ui_touch.h"
#include "ui_type.h"

#include "page_manager.h"

#include "tim_control.h"
#include "adc_control.h"
#include "dac_control.h"

void Test_Init(void)
{
    BSP_DWT_Init();
    BSP_SDRAM_Init();
    BSP_LCD_Init();
    Touch_I2C_GPIO_Config();
    GT911_Reset_Sequence();

    Tim_Control_Init();

    HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_1);

    BSP_DWT_Delay_ms(100); // 等待触摸屏稳定

    LCD_UI_Pool_Init();

    BSP_DWT_Delay_ms(100); // 等待触摸屏稳定
    
    DES_Page_Init();
	
    BSP_DWT_Delay_ms(1000); // 等待页面初始化
}

void si5351_test(void) {
    BSP_SI5351_Init();
    BSP_SI5351_SetupCLK0(1000000, 1);
    BSP_SI5351_EnableControl(0, 1);
}

void DAC_Test(void) {
    DAC_Output_Init();
    Calc_DAC_Buffer(1);
    Calc_DAC_Buffer(2);

    Control_DAC_Enable(1, 1);
    Control_DAC_Enable(2, 1);
}

void Test_Loop(void)
{
    // 这里可以添加一些测试逻辑，比如定时刷新数据等
    uint16_t x_temp = 0;
    uint16_t y_temp = 0;
    float temp_val = 0.0f;
    uint32_t temp_count = BSP_DWT_GetCounter();
    while (1) {
        if (des_config) {
            DES_Page_Init();
            while (des_config) {
                Page_Touch_Logic();
            }
        }
        if (osc_config) {
            OSC_Page_Init();
            while (osc_config) {
                Page_Touch_Logic();
            }
        }
        if (gen_config) {
            GEN_Page_Init();
            DAC_Output_Init();
            while (gen_config) {
                uint8_t changed = Page_Touch_Logic();
                GEN_Logic_Running(changed);
            }
            DAC_Output_Init();
        }
		BSP_DWT_Delay_ms(50);
    }
}
