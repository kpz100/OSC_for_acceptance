#include "lcd_control.h"
#include "lcd_control_ui.h"
#include "bsp_sdram.h"
#include "bsp_lcd_single.h"
#include "bsp_dwt.h"
#include "bsp_touch.h"

#include <string.h>
#include "tim.h"

void LCD_SDRAM_DWT_Init(void) {
    BSP_DWT_Init();
    BSP_SDRAM_Init();
    if (BSP_SDRAM_SelfTest() != 0) {
        while(1); // 0通过
    }
    BSP_LCD_Init();
    HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_1);
}

void LCD_Desktop_Init() {

}

void LCD_Osc_Init() {

}

void LCD_Gen_Init() {

}
 
