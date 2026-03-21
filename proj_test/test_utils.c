#include "test_utils.h"
#include "usart.h"
#include "tim.h"
#include "bsp_dwt.h"
#include "bsp_sdram.h"
#include "bsp_lcd_single.h"
#include "bsp_touch.h"
#include <stdio.h>

/* ============ 全局状态管理 ============ */

/** 硬件初始化状态标志 */
static uint8_t g_hardware_initialized = 0;

/* ============ 统一的printf实现 ============ */

/**
 * @brief 共用的测试输出函数
 * 替代了原来的 Test_Printf 和 LCD_Test_Printf
 */
void Test_Printf(const char *format, ...)
{
    char buffer[256];
    va_list args;
    
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (len > 0) {
        HAL_UART_Transmit(&huart1, (uint8_t *)buffer, len, 100);
    }
}

/* ============ 硬件初始化 ============ */

/**
 * @brief 获取硬件初始化状态
 */
uint8_t Test_Is_Hardware_Initialized(void)
{
    return g_hardware_initialized;
}

/**
 * @brief 通用LCD/SDRAM/触摸屏初始化
 * 
 * 自动跳过重复初始化，确保系统稳定性
 */
int Test_Init_Hardware(void)
{
    // 如果已初始化，则跳过
    if (g_hardware_initialized) {
        Test_Printf("[INFO] Hardware already initialized, skipping...\r\n");
        return 0;
    }
    
    Test_Printf("[INIT] Starting hardware initialization...\r\n");
    
    // 步骤1: 启动PWM背光
    Test_Printf("[INIT] Step 1: Starting backlight PWM...\r\n");
    if (HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_1) != HAL_OK) {
        Test_Printf("[ERROR] Failed to start backlight PWM\r\n");
        return -1;
    }
    Test_Printf("[DONE] Backlight PWM started\r\n");
    
    // 步骤2: DWT计时器初始化
    Test_Printf("[INIT] Step 2: Initializing DWT...\r\n");
    BSP_DWT_Init();
    Test_Printf("[DONE] DWT initialized\r\n");
    
    // 步骤3: SDRAM初始化
    Test_Printf("[INIT] Step 3: Initializing SDRAM...\r\n");
    BSP_SDRAM_Init();
    Test_Printf("[DONE] SDRAM initialized\r\n");
    
    // 步骤4: LCD初始化
    Test_Printf("[INIT] Step 4: Initializing LCD...\r\n");
    BSP_LCD_Init();
    Test_Printf("[DONE] LCD initialized\r\n");
    
    // 步骤5: 触摸屏I2C配置
    Test_Printf("[INIT] Step 5: Configuring touch I2C...\r\n");
    Touch_I2C_GPIO_Config();
    Test_Printf("[DONE] Touch I2C configured\r\n");
    
    // 步骤6: 触摸屏复位
    Test_Printf("[INIT] Step 6: Executing GT911 reset sequence...\r\n");
    GT911_Reset_Sequence();
    Test_Printf("[DONE] GT911 reset completed\r\n");
    
    g_hardware_initialized = 1;
    Test_Printf("[SUCCESS] All hardware initialized\r\n");
    
    return 1;
}

/* ============ 格式化输出助手 ============ */

/**
 * @brief 打印装饰性的标题
 */
void Test_Print_Title(const char *title)
{
    Test_Printf("\r\n");
    Test_Printf("========================================\r\n");
    Test_Printf("=== %s ===\r\n", title);
    Test_Printf("========================================\r\n");
}

/**
 * @brief 打印分隔符
 */
void Test_Print_Separator(uint8_t level)
{
    if (level == 1) {
        Test_Printf("\r\n--- %s ---\r\n", "");
    } else {
        Test_Printf("\r\n  :: \r\n");
    }
}

/* ============ 测试启动器 ============ */

// 前向声明（来自不同的测试模块）
void Test_LCD_Comprehensive(void);        // test.c
void Test_Touch_Comprehensive(void);      // test.c
void Test_LCD_Control_Init(void);         // test_lcd_control.c
void Test_LCD_Control_Button(void);       // test_lcd_control.c
void Test_LCD_Control_Pages(void);        // test_lcd_control.c
void Test_LCD_Control_Full(void);         // test_lcd_control.c

/**
 * @brief 统一的测试启动入口
 * 
 * 替代了原来的 Test_Start() 和 Test_Selector_Run()
 * 所有测试模块通过此函数统一启动
 */
void Test_Start(void)
{
    Test_Printf("\r\n");
    Test_Printf("╔════════════════════════════════════════╗\r\n");
    Test_Printf("║   Test Framework Started               ║\r\n");
    Test_Printf("║   Current Mode: %d                     ║\r\n", TEST_CURRENT_MODE);
    Test_Printf("╚════════════════════════════════════════╝\r\n");
    
#if TEST_CURRENT_MODE == TEST_MODE_ORIGINAL_LCD
    Test_Printf("\r\n[MODE] ORIGINAL_LCD - Testing LCD driver\r\n");
    Test_Init_Hardware();
    Test_LCD_Comprehensive();
    
#elif TEST_CURRENT_MODE == TEST_MODE_ORIGINAL_TOUCH
    Test_Printf("\r\n[MODE] ORIGINAL_TOUCH - Testing touch screen driver\r\n");
    Test_Init_Hardware();
    Test_Touch_Comprehensive();
    
#elif TEST_CURRENT_MODE == TEST_MODE_LCD_CONTROL_INIT
    Test_Printf("\r\n[MODE] LCD_CONTROL_INIT - Testing GUI framework initialization\r\n");
    Test_Init_Hardware();
    Test_LCD_Control_Init();
    
#elif TEST_CURRENT_MODE == TEST_MODE_LCD_CONTROL_BTN
    Test_Printf("\r\n[MODE] LCD_CONTROL_BTN - Testing GUI button controls\r\n");
    Test_Init_Hardware();
    Test_LCD_Control_Button();
    
#elif TEST_CURRENT_MODE == TEST_MODE_LCD_CONTROL_PAGE
    Test_Printf("\r\n[MODE] LCD_CONTROL_PAGE - Testing GUI page management\r\n");
    Test_Init_Hardware();
    Test_LCD_Control_Pages();
    
#elif TEST_CURRENT_MODE == TEST_MODE_LCD_CONTROL_FULL
    Test_Printf("\r\n[MODE] LCD_CONTROL_FULL - Full GUI integration test\r\n");
    Test_Init_Hardware();
    Test_LCD_Control_Full();
    
#else
    Test_Printf("\r\n[ERROR] Unknown test mode: %d\r\n", TEST_CURRENT_MODE);
#endif
    
    Test_Printf("\r\n");
    Test_Printf("╔════════════════════════════════════════╗\r\n");
    Test_Printf("║   All Tests Completed                  ║\r\n");
    Test_Printf("╚════════════════════════════════════════╝\r\n");
    Test_Printf("\r\n");
}
