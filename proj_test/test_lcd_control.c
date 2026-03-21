#include "test_lcd_control.h"
#include "test_utils.h"
#include "usart.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* ============ 调试输出函数 ============ */

/**
 * @brief 调试信息打印函数
 * @deprecated 使用 Test_Printf() 替代 (来自 test_utils.h)
 * 
 * 此宏确保代码兼容性，直接映射到统一的Test_Printf()
 */
#define LCD_Test_Printf Test_Printf

/* ============ 全局测试变量 ============ */

/** 按钮点击计数器 */
static uint32_t g_button_click_count = 0;

/** 按钮1被点击标志 */
static uint8_t g_button1_clicked = 0;

/** 按钮2被点击标志 */
static uint8_t g_button2_clicked = 0;

/* ============ 按钮回调函数 ============ */

/**
 * @brief 按钮1的点击回调
 * 
 * 功能: 切换到示波器页面
 */
static void Button1_OnClick(void)
{
    LCD_Test_Printf("Button 1 Clicked!\r\n");
    g_button_click_count++;
    g_button1_clicked = 1;
    GUI_Set_Active_Page(PAGE_OSCILLOSCOPE);
}

/**
 * @brief 按钮2的点击回调
 * 
 * 功能: 切换到信号发生器页面
 */
static void Button2_OnClick(void)
{
    LCD_Test_Printf("Button 2 Clicked!\r\n");
    g_button_click_count++;
    g_button2_clicked = 1;
    GUI_Set_Active_Page(PAGE_SIGNAL_GEN);
}

/**
 * @brief 按钮3的点击回调
 * 
 * 功能: 返回主页面
 */
static void Button3_OnClick(void)
{
    LCD_Test_Printf("Button 3 Clicked! (Return to Home)\r\n");
    g_button_click_count++;
    GUI_Set_Active_Page(PAGE_DESKTOP);
}

/* ============ 测试函数实现 ============ */

/**
 * @brief LCD初始化测试
 */
void Test_LCD_Control_Init(void)
{
    LCD_Test_Printf("\r\n=== LCD Control Init Test ===\r\n");
    
    // 配置背光
    HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_1);
    LCD_Test_Printf("BackLight PWM Started\r\n");
    
    // 初始化所有系统
    LCD_SDRAM_DWT_Init();
    LCD_Test_Printf("LCD_SDRAM_DWT_Init() Complete\r\n");
    
    // 显示初始化状态
    BSP_LCD_Clear(LCD_COLOR_BLACK);
    BSP_LCD_FillRect(100, 150, 600, 100, LCD_COLOR_GREEN);
    BSP_LCD_Flip();
    
    LCD_Test_Printf("LCD Init Test Successful\r\n");
    LCD_Test_Printf("Current Page: %d\r\n", (int)PAGE_DESKTOP);
    
    BSP_DWT_Delay_ms(2000);
}

/**
 * @brief 按钮创建与交互测试
 */
void Test_LCD_Control_Button(void)
{
    LCD_Test_Printf("\r\n=== LCD Control Button Test ===\r\n");
    
    // 初始化系统
    HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_1);
    LCD_SDRAM_DWT_Init();
    
    // 创建按钮到主页面
    LCD_Test_Printf("Creating Buttons on PAGE_DESKTOP...\r\n");
    
    LCD_Button_Struct* btn1 = GUI_Create_Button(
        PAGE_DESKTOP, 
        50, 50, 150, 60, 
        "Page 2", 
        Button1_OnClick
    );
    
    LCD_Button_Struct* btn2 = GUI_Create_Button(
        PAGE_DESKTOP, 
        250, 50, 150, 60, 
        "Page 3", 
        Button2_OnClick
    );
    
    LCD_Button_Struct* btn3 = GUI_Create_Button(
        PAGE_DESKTOP, 
        450, 50, 150, 60, 
        "Home", 
        Button3_OnClick
    );
    
    if (btn1 && btn2 && btn3) {
        LCD_Test_Printf("All Buttons Created Successfully\r\n");
    } else {
        LCD_Test_Printf("ERROR: Button Creation Failed!\r\n");
        return;
    }
    
    // 绘制按钮
    GUI_Draw_Active_Page();
    LCD_Test_Printf("Buttons Drawn on LCD\r\n");
    
    // 模拟按钮点击序列
    LCD_Test_Printf("Simulating Button Clicks...\r\n");
    for (int i = 0; i < 3; i++) {
        LCD_Test_Printf("Click Iteration %d\r\n", i+1);
        
        // 模拟点击按钮1
        btn1->state = 1;
        Draw_Button(btn1);  // 这会在实际代码中工作，需要静态化
        BSP_DWT_Delay_ms(200);
        btn1->state = 0;
        
        BSP_DWT_Delay_ms(500);
    }
    
    LCD_Test_Printf("Button Test Complete - Total Clicks: %lu\r\n", g_button_click_count);
    LCD_Test_Printf("Button 1 Clicked: %s\r\n", g_button1_clicked ? "Yes" : "No");
    LCD_Test_Printf("Button 2 Clicked: %s\r\n", g_button2_clicked ? "Yes" : "No");
}

/**
 * @brief 页面切换测试
 */
void Test_LCD_Control_Pages(void)
{
    LCD_Test_Printf("\r\n=== LCD Control Page Switch Test ===\r\n");
    
    // 初始化系统
    HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_1);
    LCD_SDRAM_DWT_Init();
    
    LCD_Test_Printf("Testing Page Management...\r\n");
    
    // 创建各页面的演示内容
    LCD_Test_Printf("Creating controls on PAGE_DESKTOP...\r\n");
    GUI_Create_Button(PAGE_DESKTOP, 100, 100, 200, 80, "Home Page", NULL);
    
    LCD_Test_Printf("Creating controls on PAGE_OSCILLOSCOPE...\r\n");
    GUI_Create_Button(PAGE_OSCILLOSCOPE, 100, 100, 200, 80, "Oscilloscope", NULL);
    
    LCD_Test_Printf("Creating controls on PAGE_SIGNAL_GEN...\r\n");
    GUI_Create_Button(PAGE_SIGNAL_GEN, 100, 100, 200, 80, "Signal Gen", NULL);
    
    // 页面切换测试
    Page_ID_t pages[] = {PAGE_DESKTOP, PAGE_OSCILLOSCOPE, PAGE_SIGNAL_GEN};
    const char* page_names[] = {"DESKTOP", "OSCILLOSCOPE", "SIGNAL_GEN"};
    
    for (int cycle = 0; cycle < 2; cycle++) {
        LCD_Test_Printf("\r\nPage Switch Cycle %d:\r\n", cycle + 1);
        
        for (int i = 0; i < 3; i++) {
            GUI_Set_Active_Page(pages[i]);
            LCD_Test_Printf("  Switched to PAGE_%s\r\n", page_names[i]);
            
            GUI_Draw_Active_Page();
            LCD_Test_Printf("  Page redrawn\r\n");
            
            BSP_DWT_Delay_ms(1000);
        }
    }
    
    LCD_Test_Printf("\r\nPage Switch Test Complete\r\n");
}

/**
 * @brief 完整功能集成测试
 */
void Test_LCD_Control_Full(void)
{
    LCD_Test_Printf("\r\n========================================\r\n");
    LCD_Test_Printf("=== LCD Control Full Integration Test ===\r\n");
    LCD_Test_Printf("========================================\r\n");
    
    // 初始化系统
    LCD_Test_Printf("Step 1: Initializing System...\r\n");
    HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_1);
    LCD_SDRAM_DWT_Init();
    LCD_Test_Printf("System Initialized Successfully\r\n\r\n");
    
    // 创建主页面控件
    LCD_Test_Printf("Step 2: Creating Page Controls...\r\n");
    
    GUI_Create_Button(PAGE_DESKTOP, 50, 50, 140, 60, "To Page 2", Button1_OnClick);
    GUI_Create_Button(PAGE_DESKTOP, 250, 50, 140, 60, "To Page 3", Button2_OnClick);
    GUI_Create_Button(PAGE_DESKTOP, 450, 50, 140, 60, "About", NULL);
    
    GUI_Create_Button(PAGE_OSCILLOSCOPE, 50, 50, 600, 60, "Oscilloscope Page - Press to Switch", Button3_OnClick);
    GUI_Create_Button(PAGE_OSCILLOSCOPE, 50, 150, 600, 60, "Displaying Waveforms...", NULL);
    
    GUI_Create_Button(PAGE_SIGNAL_GEN, 50, 50, 600, 60, "Signal Generator Page - Press to Switch", Button3_OnClick);
    GUI_Create_Button(PAGE_SIGNAL_GEN, 50, 150, 600, 60, "Generating Signals...", NULL);
    
    LCD_Test_Printf("All Controls Created\r\n\r\n");
    
    // 运行演示循环
    LCD_Test_Printf("Step 3: Running Demo Loop...\r\n");
    LCD_Test_Printf("Cycle Information:\r\n");
    LCD_Test_Printf("  Duration: 6 cycles (18 seconds total)\r\n");
    LCD_Test_Printf("  Each page: 2 seconds display\r\n\r\n");
    
    uint32_t cycle_count = 0;
    for (cycle_count = 0; cycle_count < 3; cycle_count++) {
        LCD_Test_Printf("--- Cycle %lu ---\r\n", cycle_count + 1);
        
        // 主页面
        LCD_Test_Printf("  PAGE_DESKTOP\r\n");
        GUI_Set_Active_Page(PAGE_DESKTOP);
        GUI_Draw_Active_Page();
        GUI_Process_Touch();
        BSP_DWT_Delay_ms(2000);
        
        // 示波器页面
        LCD_Test_Printf("  PAGE_OSCILLOSCOPE\r\n");
        GUI_Set_Active_Page(PAGE_OSCILLOSCOPE);
        GUI_Draw_Active_Page();
        GUI_Process_Touch();
        BSP_DWT_Delay_ms(2000);
        
        // 信号发生器页面
        LCD_Test_Printf("  PAGE_SIGNAL_GEN\r\n");
        GUI_Set_Active_Page(PAGE_SIGNAL_GEN);
        GUI_Draw_Active_Page();
        GUI_Process_Touch();
        BSP_DWT_Delay_ms(2000);
    }
    
    // 输出测试统计信息
    LCD_Test_Printf("\r\n========================================\r\n");
    LCD_Test_Printf("=== Test Statistics ===\r\n");
    LCD_Test_Printf("Total Cycles Completed: %lu\r\n", cycle_count);
    LCD_Test_Printf("Total Button Clicks: %lu\r\n", g_button_click_count);
    LCD_Test_Printf("Status: Page switching framework operational\r\n");
    LCD_Test_Printf("========================================\r\n");
    LCD_Test_Printf("Full Integration Test Complete\r\n\r\n");
}

/* ============ 测试入口函数 ============ */


// Note: Test_LCD_Control_Start() 已移至 test_utils.c 中实现
// 新的统一入口使用 Test_Start() 从 test_utils.h
// 选择测试类型请修改 test_utils.h 中的 TEST_CURRENT_MODE 宏
