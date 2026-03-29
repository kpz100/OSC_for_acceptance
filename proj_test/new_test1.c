#include "new_test1.h"
#include "osc_manager.h"
#include "bsp_dwt.h"
#include "bsp_sdram.h"
#include "bsp_lcd_single.h"
#include "bsp_touch.h"
#include "tim.h"
#include "ascii_font.h"
#include <stdio.h>
#include <string.h>

#include "ui_rampool.h"
#include "ui_render.h"
#include "ui_touch.h"
#include "ui_type.h"

void Test_Init(void)
{
    BSP_DWT_Init();
    BSP_SDRAM_Init();
    BSP_LCD_Init();
    Touch_I2C_GPIO_Config();
    GT911_Reset_Sequence();

    HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_1);

    BSP_DWT_Delay_ms(100); // 等待触摸屏稳定

    LCD_UI_Pool_Init();

    BSP_DWT_Delay_ms(100); // 等待触摸屏稳定
    
    OSC_Page_Init();
    UI_Render_All();

    BSP_DWT_Delay_ms(1000); // 等待页面初始化
    OSC_Page_Refresh_Data(1.23f, 50.0f, 2.34f, 60.0f);
}

void Test_Loop(void)
{
    // 这里可以添加一些测试逻辑，比如定时刷新数据等
    uint16_t x_temp = 0;
    uint16_t y_temp = 0;
    while (1) {
        if (GT911_Scan()) {
            // 处理触摸事件
            for (uint8_t i = 0; i < touch_data.touch_num; i++) {
                x_temp += touch_data.x[i];
                y_temp += touch_data.y[i];
            }
            x_temp /= touch_data.touch_num;
            y_temp /= touch_data.touch_num;

            UI_Touch_Process(x_temp, y_temp);
            printf("Touch at: (%d, %d)\n", x_temp, y_temp);
            x_temp = 0;
            y_temp = 0;
        }
        BSP_DWT_Delay_ms(50); // 减少 CPU 占用
    }
}
