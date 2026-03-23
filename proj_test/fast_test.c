#include "fast_test.h"
#include "tim.h"

/**
 * @brief 简单的硬件自检逻辑，通过 printf 观察串口输出
 */
void Fast_Test_Run(void) 
{
    printf("\r\n========== BSP Hardware Test Start ==========\r\n");

    /* 1. 初始化 DWT (用于高精度延时和测量) */
    BSP_DWT_Init();
    printf("[1/4] DWT Timer: Initialized.\r\n");

    /* 2. 初始化并测试 SDRAM */
    // 注意：SDRAM 是显存的基础，如果它不稳，LCD 就会花屏
    BSP_SDRAM_Init();
    printf("[2/4] SDRAM: Initializing...\r\n");
    if (BSP_SDRAM_SelfTest() == 0) {
        printf("      Result: SDRAM Check Passed!\r\n");
    } else {
        printf("      Result: SDRAM Check FAILED! Check your hardware or FMC config.\r\n");
        // 如果 SDRAM 挂了，后面测试没意义，建议卡死在这里
        while(1); 
    }

    /* 3. 初始化 LCD 并进行视觉测试 */
	HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_1);
    printf("[3/4] LCD: Initializing LTDC & DMA2D...\r\n");
    BSP_LCD_Init(); 
    
    // 循环切换颜色，确保显示控制正常
    printf("      Action: Cycling screen colors (Red -> Green -> Blue)...\r\n");
    BSP_LCD_Clear(LCD_COLOR_RED);   BSP_DWT_Delay_ms(500);
    BSP_LCD_Clear(LCD_COLOR_GREEN); BSP_DWT_Delay_ms(500);
    BSP_LCD_Clear(LCD_COLOR_BLUE);  BSP_DWT_Delay_ms(500);
    
    // 画一个白色的矩形框，确认坐标系统
    BSP_LCD_FillRect(100, 100, 600, 280, LCD_COLOR_WHITE);
    printf("      Action: White rectangle drawn in the center.\r\n");

    /* 4. 触摸屏测试 */
    printf("[4/4] Touch: Initializing GT911...\r\n");
    Touch_I2C_GPIO_Config();
    GT911_Reset_Sequence();
    
    printf("      Action: Entering Touch Test Loop. Touch the screen to see coordinates.\r\n");
    printf("      (Press Reset on board to exit this test)\r\n\r\n");

    while (1) 
    {
        // 扫描触摸屏
        if (GT911_Scan() == 1) 
        {
            // 如果有触摸，打印第一个点坐标
            printf("Touch Detected! P0 -> X:%d, Y:%d | Points:%d\r\n", 
                    touch_data.x[0], touch_data.y[0], touch_data.touch_num);
            
            // 在触摸位置画一个小点作为视觉反馈（注意：如果刷得太快会影响 I2C 稳定性）
            BSP_LCD_FillRect(touch_data.x[0], touch_data.y[0], 10, 10, LCD_COLOR_YELLOW);
        }
        
        // 稍微延时，防止串口打印过快挂掉
        HAL_Delay(20); 
    }
}