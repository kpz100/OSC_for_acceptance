#include "test_all.h"
#include "lcd_control.h"
#include "bsp_lcd_single.h"
#include "bsp_touch.h"
#include "bsp_dwt.h"
#include "usart.h"
#include "tim.h"
#include "bsp_sdram.h"
#include "ascii_font.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* ==================== 测试框架配置 ==================== */

/**
 * @defgroup TEST_MODES 测试类型定义
 * @{
 */

/** 原始LCD功能测试 - 测试LCD驱动的基础功能 */
#define TEST_MODE_ORIGINAL_LCD      0

/** 原始触摸屏测试 - 测试GT911触摸屏驱动 */
#define TEST_MODE_ORIGINAL_TOUCH    1

/** LCD控制框架初始化测试 - 测试GUI框架初始化 */
#define TEST_MODE_LCD_CONTROL_INIT  2

/** LCD控制框架按钮测试 - 测试GUI按钮控件 */
#define TEST_MODE_LCD_CONTROL_BTN   3

/** LCD控制框架页面测试 - 测试GUI页面管理 */
#define TEST_MODE_LCD_CONTROL_PAGE  4

/** LCD控制框架完整测试 - 完整的GUI集成测试 */
#define TEST_MODE_LCD_CONTROL_FULL  5

/** @} */

/** 
 * @brief 当前测试模式 - 在此处选择要运行的测试
 * 
 * 使用 TEST_MODE_* 常量之一
 * 修改此值可快速切换测试
 */
#define TEST_CURRENT_MODE           TEST_MODE_ORIGINAL_LCD

/* ==================== 全局状态管理 ==================== */

/** 硬件初始化状态标志 */
static uint8_t g_hardware_initialized = 0;

/* ==================== 统一的printf实现 ==================== */

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

/* ==================== 硬件初始化 ==================== */

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

/* ==================== 格式化输出助手 ==================== */

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

/* ==================== LCD驱动测试 ==================== */

/**
 * @brief 测试LCD颜色显示
 */
static void Test_LCD_Colors(void)
{
    uint32_t colors[] = {
        LCD_COLOR_BLACK,
        LCD_COLOR_RED,
        LCD_COLOR_GREEN,
        LCD_COLOR_BLUE,
        LCD_COLOR_WHITE,
        LCD_COLOR_CYAN,
        LCD_COLOR_MAGENTA,
        LCD_COLOR_YELLOW,
        LCD_COLOR_DARKGREEN
    };
    
    Test_Printf("Testing LCD Colors...\r\n");
    
    uint16_t rect_width = 50;
    uint16_t rect_height = 50;
    uint16_t start_x = 50;
    uint16_t start_y = 50;
    
    BSP_LCD_Clear(LCD_COLOR_BLACK);
    
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            uint16_t x = start_x + j * (rect_width + 20);
            uint16_t y = start_y + i * (rect_height + 30);
            BSP_LCD_FillRect(x, y, rect_width, rect_height, colors[i * 3 + j]);
        }
    }
    
    BSP_LCD_Flip();
    BSP_DWT_Delay_ms(2000);
}

/**
 * @brief 测试LCD像素绘制
 */
static void Test_LCD_Pixels(void)
{
    Test_Printf("Testing LCD Pixel Drawing...\r\n");
    
    BSP_LCD_Clear(LCD_COLOR_BLACK);
    
    // 绘制对角线
    for (int i = 0; i < 400; i++)
    {
        BSP_LCD_DrawPixel(100 + i / 2, 100 + i, LCD_COLOR_GREEN);
    }
    
    // 绘制竖线
    for (int i = 0; i < 100; i++)
    {
        BSP_LCD_DrawPixel(400, 200 + i, LCD_COLOR_BLUE);
    }
    
    // 绘制横线
    for (int i = 0; i < 150; i++)
    {
        BSP_LCD_DrawPixel(250 + i, 350, LCD_COLOR_RED);
    }
    
    BSP_LCD_Flip();
    BSP_DWT_Delay_ms(2000);
}

/**
 * @brief 测试LCD梯度效果
 */
static void Test_LCD_Gradient(void)
{
    Test_Printf("Testing LCD Gradient...\r\n");
    
    BSP_LCD_Clear(LCD_COLOR_BLACK);
    
    // 红色梯度
    for (int x = 0; x < 200; x++)
    {
        uint8_t red = (x * 255) / 200;
        uint32_t color = 0xFF000000 | (red << 16);
        BSP_LCD_FillRect(x, 0, 1, 480, color);
    }
    
    // 绿色梯度
    for (int x = 200; x < 400; x++)
    {
        uint8_t green = ((x - 200) * 255) / 200;
        uint32_t color = 0xFF000000 | (green << 8);
        BSP_LCD_FillRect(x, 0, 1, 480, color);
    }
    
    // 蓝色梯度
    for (int x = 400; x < 600; x++)
    {
        uint8_t blue = ((x - 400) * 255) / 200;
        uint32_t color = 0xFF000000 | blue;
        BSP_LCD_FillRect(x, 0, 1, 480, color);
    }
    
    // 白色梯度
    for (int x = 600; x < 800; x++)
    {
        uint8_t gray = ((x - 600) * 255) / 200;
        uint32_t color = 0xFF000000 | (gray << 16) | (gray << 8) | gray;
        BSP_LCD_FillRect(x, 0, 1, 480, color);
    }
    
    BSP_LCD_Flip();
    BSP_DWT_Delay_ms(2000);
}

/**
 * @brief 测试LCD棋盘图案
 */
static void Test_LCD_Checkerboard(void)
{
    Test_Printf("Testing LCD Checkerboard Pattern...\r\n");
    
    BSP_LCD_Clear(LCD_COLOR_BLACK);
    
    uint16_t square_size = 40;
    uint32_t colors[] = {LCD_COLOR_WHITE, LCD_COLOR_BLACK};
    
    for (uint16_t y = 0; y < 480; y += square_size)
    {
        for (uint16_t x = 0; x < 800; x += square_size)
        {
            uint32_t color_index = ((x / square_size) + (y / square_size)) % 2;
            BSP_LCD_FillRect(x, y, square_size, square_size, colors[color_index]);
        }
    }
    
    BSP_LCD_Flip();
    BSP_DWT_Delay_ms(2000);
}

/**
 * @brief 测试LCD填充操作
 */
static void Test_LCD_Fills(void)
{
    Test_Printf("Testing LCD Fill Operations...\r\n");
    
    uint32_t fill_colors[] = {
        LCD_COLOR_RED,
        LCD_COLOR_GREEN,
        LCD_COLOR_BLUE,
        LCD_COLOR_YELLOW,
        LCD_COLOR_CYAN,
        LCD_COLOR_MAGENTA
    };
    
    for (int i = 0; i < 6; i++)
    {
        BSP_LCD_Clear(fill_colors[i]);
        BSP_LCD_Flip();
        BSP_DWT_Delay_ms(1000);
    }
}

/**
 * @brief 测试LCD缓冲翻转
 */
static void Test_LCD_Flipping(void)
{
    Test_Printf("Testing LCD Buffer Flipping...\r\n");
    
    BSP_LCD_Clear(LCD_COLOR_BLACK);
    BSP_LCD_FillRect(100, 100, 200, 100, LCD_COLOR_RED);
    BSP_LCD_Flip();
    BSP_DWT_Delay_ms(1000);
    
    BSP_LCD_Clear(LCD_COLOR_BLACK);
    BSP_LCD_FillRect(500, 200, 200, 100, LCD_COLOR_GREEN);
    BSP_LCD_Flip();
    BSP_DWT_Delay_ms(1000);
    
    BSP_LCD_Clear(LCD_COLOR_BLACK);
    BSP_LCD_FillRect(300, 300, 200, 100, LCD_COLOR_BLUE);
    BSP_LCD_Flip();
    BSP_DWT_Delay_ms(1000);
}

/**
 * @brief 测试LCD文字绘制 (8x16字体)
 */
static void Test_LCD_String_8x16(void)
{
    Test_Printf("Testing LCD String Drawing (8x16 Font)...\r\n");
    
    BSP_LCD_Clear(LCD_COLOR_BLACK);
    
    // 绘制标题
    BSP_LCD_DrawString(10, 10, "LCD String Test", LCD_COLOR_WHITE, ASCII_FONT_TYPE_8x16);
    
    // 绘制多行文本
    BSP_LCD_DrawString(10, 40, "Line 1: Red Text", LCD_COLOR_RED, ASCII_FONT_TYPE_8x16);
    BSP_LCD_DrawString(10, 70, "Line 2: Green Text", LCD_COLOR_GREEN, ASCII_FONT_TYPE_8x16);
    BSP_LCD_DrawString(10, 100, "Line 3: Blue Text", LCD_COLOR_BLUE, ASCII_FONT_TYPE_8x16);
    BSP_LCD_DrawString(10, 130, "Line 4: Yellow Text", LCD_COLOR_YELLOW, ASCII_FONT_TYPE_8x16);
    BSP_LCD_DrawString(10, 160, "Line 5: Cyan Text", LCD_COLOR_CYAN, ASCII_FONT_TYPE_8x16);
    BSP_LCD_DrawString(10, 190, "Line 6: Magenta Text", LCD_COLOR_MAGENTA, ASCII_FONT_TYPE_8x16);
    
    // 绘制特殊字符
    BSP_LCD_DrawString(10, 220, "!@#$%^&*()_+-=[]{}|;", LCD_COLOR_WHITE, ASCII_FONT_TYPE_8x16);
    
    BSP_LCD_Flip();
    BSP_DWT_Delay_ms(3000);
}

/**
 * @brief 测试LCD文字绘制 (16x32字体)
 */
static void Test_LCD_String_16x32(void)
{
    Test_Printf("Testing LCD String Drawing (16x32 Font)...\r\n");
    
    BSP_LCD_Clear(LCD_COLOR_BLACK);
    
    // 绘制大字体文本
    BSP_LCD_DrawString(50, 50, "Hello", LCD_COLOR_WHITE, ASCII_FONT_TYPE_16x32);
    BSP_LCD_DrawString(50, 100, "World", LCD_COLOR_YELLOW, ASCII_FONT_TYPE_16x32);
    BSP_LCD_DrawString(50, 150, "Test", LCD_COLOR_CYAN, ASCII_FONT_TYPE_16x32);
    
    BSP_LCD_Flip();
    BSP_DWT_Delay_ms(3000);
}

/**
 * @brief 测试LCD文字绘制综合
 */
static void Test_LCD_String_Combined(void)
{
    Test_Printf("Testing LCD String Drawing (Combined Fonts)...\r\n");
    
    BSP_LCD_Clear(LCD_COLOR_BLUE);
    
    // 标题 (16x32)
    BSP_LCD_DrawString(100, 20, "STM32H743", LCD_COLOR_WHITE, ASCII_FONT_TYPE_16x32);
    
    // 小字体信息 (8x16)
    BSP_LCD_DrawString(20, 120, "Model: STM32H743IIT6", LCD_COLOR_YELLOW, ASCII_FONT_TYPE_8x16);
    BSP_LCD_DrawString(20, 150, "Board: H743II SDRAM A1", LCD_COLOR_YELLOW, ASCII_FONT_TYPE_8x16);
    BSP_LCD_DrawString(20, 180, "LCD: 800x480 ARGB8888", LCD_COLOR_YELLOW, ASCII_FONT_TYPE_8x16);
    BSP_LCD_DrawString(20, 210, "Touch: GT911 Multi-touch", LCD_COLOR_YELLOW, ASCII_FONT_TYPE_8x16);
    BSP_LCD_DrawString(20, 240, "Font Test: Small&Large", LCD_COLOR_GREEN, ASCII_FONT_TYPE_8x16);
    
    // 底部版权信息 (8x16)
    BSP_LCD_DrawString(150, 430, "Copyright 2026", LCD_COLOR_CYAN, ASCII_FONT_TYPE_8x16);
    
    BSP_LCD_Flip();
    BSP_DWT_Delay_ms(3000);
}

/**
 * @brief LCD综合测试入口
 */
void Test_LCD_Comprehensive(void)
{
    Test_Print_Title("Comprehensive LCD Test");
    
    Test_Init_Hardware();
    BSP_DWT_Delay_ms(500);
    
    for (int i = 0; i < 100; i++) {
        Test_LCD_Colors();
        Test_LCD_Pixels();
        Test_LCD_Gradient();
        Test_LCD_Checkerboard();
        Test_LCD_Fills();
        Test_LCD_Flipping();
        Test_LCD_String_8x16();
        Test_LCD_String_16x32();
        Test_LCD_String_Combined();
        
        BSP_LCD_Clear(LCD_COLOR_BLACK);
        BSP_LCD_FillRect(250, 200, 300, 80, LCD_COLOR_GREEN);
        BSP_LCD_Flip();
        
        Test_Printf("=== Cycle %d OK ===\r\n\r\n", i + 1);
        BSP_DWT_Delay_ms(1000);
    }
}

/* ==================== 触摸屏驱动测试 ==================== */

/**
 * @brief 测试GT911初始化
 */
static void Test_Touch_Init(void)
{
    Test_Printf("Testing GT911 Touch Sensor Initialization...\r\n");
    
    Touch_I2C_GPIO_Config();
    Test_Printf("I2C Bus Initialized\r\n");
    
    GT911_Reset_Sequence();
    Test_Printf("GT911 Reset Sequence Completed\r\n");
    
    BSP_DWT_Delay_ms(100);
}

/**
 * @brief 测试单次触摸扫描
 */
static void Test_Touch_Scan(void)
{
    Test_Printf("Testing GT911 Touch Sensor Scanning...\r\n");
    
    uint8_t result = GT911_Scan();
    
    if (result)
    {
        Test_Printf("Touch Detected: ");
        Test_Printf("Touch Count = %d\r\n", touch_data.touch_num);
        
        for (uint8_t i = 0; i < touch_data.touch_num; i++)
        {
            Test_Printf("  Touch %d: X=%d, Y=%d, Size=%d\r\n", 
                        i+1, touch_data.x[i], touch_data.y[i], touch_data.size[i]);
        }
    }
    else
    {
        Test_Printf("No Touch Detected\r\n");
    }
}

/**
 * @brief 测试I2C通信
 */
static void Test_Touch_I2C(void)
{
    Test_Printf("Testing GT911 I2C Communication...\r\n");
    
    uint8_t test_data[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    uint8_t read_data[4] = {0};
    
    GT911_WR_Reg(GT_CFG_REG, test_data, 4);
    Test_Printf("Wrote test data to configuration register\r\n");
    
    BSP_DWT_Delay_ms(10);
    
    GT911_RD_Reg(GT_CFG_REG, read_data, 4);
    Test_Printf("Read data: 0x%02X, 0x%02X, 0x%02X, 0x%02X\r\n",
                read_data[0], read_data[1], read_data[2], read_data[3]);
}

/**
 * @brief 测试连续触摸轮询
 */
static void Test_Touch_Polling(uint32_t duration_ms)
{
    Test_Printf("Testing Continuous Touch Polling (%lu ms)...\r\n", duration_ms);
    
    uint32_t start_time = HAL_GetTick();
    uint32_t scan_count = 0;
    uint32_t touch_events = 0;
    
    while ((HAL_GetTick() - start_time) < duration_ms)
    {
        if (GT911_Scan())
        {
            touch_events++;
            Test_Printf("Touch Event %lu: Touches=%d, X[0]=%d, Y[0]=%d\r\n",
                        touch_events, touch_data.touch_num, touch_data.x[0], touch_data.y[0]);
        }
        scan_count++;
        BSP_DWT_Delay_ms(50);
    }
    
    Test_Printf("Polling Complete: Total Scans=%lu, Touch Events=%lu\r\n", scan_count, touch_events);
}

/**
 * @brief 测试多点触摸能力
 */
static void Test_Touch_MultiTouch(uint32_t duration_ms)
{
    Test_Printf("Testing Multi-Touch Capability (%lu ms)...\r\n", duration_ms);
    
    uint32_t start_time = HAL_GetTick();
    uint32_t max_touches = 0;
    
    while ((HAL_GetTick() - start_time) < duration_ms)
    {
        if (GT911_Scan())
        {
            if (touch_data.touch_num > max_touches)
            {
                max_touches = touch_data.touch_num;
                Test_Printf("%lu simultaneous touches detected:\r\n", max_touches);
                
                for (uint8_t i = 0; i < max_touches; i++)
                {
                    Test_Printf("  Touch %d: X=%d, Y=%d, Size=%d\r\n",
                                i+1, touch_data.x[i], touch_data.y[i], touch_data.size[i]);
                }
            }
        }
        BSP_DWT_Delay_ms(50);
    }
    
    Test_Printf("Maximum simultaneous touches detected: %lu\r\n", max_touches);
}

/**
 * @brief 触摸综合测试入口
 */
void Test_Touch_Comprehensive(void)
{
    Test_Print_Title("Comprehensive Touch Test");
    
    Test_Init_Hardware();
    BSP_DWT_Delay_ms(500);
    
    Test_Touch_Init();
    BSP_DWT_Delay_ms(500);
    
    for (uint32_t test_count = 0; test_count < 1; test_count++)
    {
        Test_Printf("\r\n--- Touch Test Iteration %lu ---\r\n", test_count + 1);
        
        Test_Touch_I2C();
        BSP_DWT_Delay_ms(500);
        
        Test_Touch_Scan();
        BSP_DWT_Delay_ms(500);
        
        Test_Touch_Polling(5000);
        BSP_DWT_Delay_ms(500);
        
        Test_Touch_MultiTouch(5000);
        BSP_DWT_Delay_ms(1000);
    }
    
    Test_Printf("\r\nComprehensive Touch Test Completed\r\n");
}

/* ==================== LCD控制框架测试 ==================== */

/** 按钮点击计数器 */
static uint32_t g_button_click_count = 0;

static void Button1_OnClick(void)
{
    Test_Printf("Button 1 Clicked!\r\n");
    g_button_click_count++;
    GUI_Set_Active_Page(PAGE_OSCILLOSCOPE);
}

static void Button2_OnClick(void)
{
    Test_Printf("Button 2 Clicked!\r\n");
    g_button_click_count++;
    GUI_Set_Active_Page(PAGE_SIGNAL_GEN);
}

static void Button3_OnClick(void)
{
    Test_Printf("Button 3 Clicked! (Return to Home)\r\n");
    g_button_click_count++;
    GUI_Set_Active_Page(PAGE_DESKTOP);
}

/**
 * @brief LCD控制框架初始化测试
 */
void Test_LCD_Control_Init(void)
{
    Test_Print_Title("LCD Control Framework Init Test");
    
    HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_1);
    LCD_SDRAM_DWT_Init();
    Test_Printf("LCD_SDRAM_DWT_Init() Complete\r\n");
    
    BSP_LCD_Clear(LCD_COLOR_BLACK);
    BSP_LCD_FillRect(100, 150, 600, 100, LCD_COLOR_GREEN);
    BSP_LCD_Flip();
    
    Test_Printf("LCD Init Test Successful\r\n");
    BSP_DWT_Delay_ms(2000);
}

/**
 * @brief 按钮控件测试
 */
void Test_LCD_Control_Button(void)
{
    Test_Print_Title("LCD Control Button Test");
    
    HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_1);
    LCD_SDRAM_DWT_Init();
    
    Test_Printf("Creating Buttons on PAGE_DESKTOP...\r\n");
    
    LCD_Button_Struct* btn1 = GUI_Create_Button(
        PAGE_DESKTOP, 50, 50, 150, 60, "Page 2", Button1_OnClick
    );
    LCD_Button_Struct* btn2 = GUI_Create_Button(
        PAGE_DESKTOP, 250, 50, 150, 60, "Page 3", Button2_OnClick
    );
    LCD_Button_Struct* btn3 = GUI_Create_Button(
        PAGE_DESKTOP, 450, 50, 150, 60, "Home", Button3_OnClick
    );
    
    if (btn1 && btn2 && btn3) {
        Test_Printf("All Buttons Created Successfully\r\n");
    } else {
        Test_Printf("ERROR: Button Creation Failed!\r\n");
        return;
    }
    
    GUI_Draw_Active_Page();
    Test_Printf("Buttons Drawn on LCD\r\n");
    
    Test_Printf("Total Clicks: %lu\r\n", g_button_click_count);
    BSP_DWT_Delay_ms(2000);
}

/**
 * @brief 页面切换测试
 */
void Test_LCD_Control_Pages(void)
{
    Test_Print_Title("LCD Control Page Switch Test");
    
    HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_1);
    LCD_SDRAM_DWT_Init();
    
    Test_Printf("Testing Page Management...\r\n");
    
    GUI_Create_Button(PAGE_DESKTOP, 100, 100, 200, 80, "Home", NULL);
    GUI_Create_Button(PAGE_OSCILLOSCOPE, 100, 100, 200, 80, "OSC", NULL);
    GUI_Create_Button(PAGE_SIGNAL_GEN, 100, 100, 200, 80, "Gen", NULL);
    
    Page_ID_t pages[] = {PAGE_DESKTOP, PAGE_OSCILLOSCOPE, PAGE_SIGNAL_GEN};
    const char* page_names[] = {"DESKTOP", "OSCILLOSCOPE", "SIGNAL_GEN"};
    
    for (int cycle = 0; cycle < 2; cycle++) {
        Test_Printf("\r\nPage Switch Cycle %d:\r\n", cycle + 1);
        
        for (int i = 0; i < 3; i++) {
            GUI_Set_Active_Page(pages[i]);
            Test_Printf("  Switched to PAGE_%s\r\n", page_names[i]);
            
            GUI_Draw_Active_Page();
            BSP_DWT_Delay_ms(1000);
        }
    }
    
    Test_Printf("Page Switch Test Complete\r\n");
}

/**
 * @brief 完整GUI测试
 */
void Test_LCD_Control_Full(void)
{
    Test_Print_Title("LCD Control Full Integration Test");
    
    HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_1);
    LCD_SDRAM_DWT_Init();
    
    Test_Printf("Creating Page Controls...\r\n");
    
    GUI_Create_Button(PAGE_DESKTOP, 50, 50, 140, 60, "Page 2", Button1_OnClick);
    GUI_Create_Button(PAGE_DESKTOP, 250, 50, 140, 60, "Page 3", Button2_OnClick);
    GUI_Create_Button(PAGE_OSCILLOSCOPE, 50, 50, 600, 60, "OSC", Button3_OnClick);
    GUI_Create_Button(PAGE_SIGNAL_GEN, 50, 50, 600, 60, "GEN", Button3_OnClick);
    
    Test_Printf("All Controls Created\r\n\r\n");
    Test_Printf("Running Demo Loop (3 cycles, 18 seconds total)...\r\n");
    
    uint32_t cycle_count = 0;
    for (cycle_count = 0; cycle_count < 3; cycle_count++) {
        Test_Printf("--- Cycle %lu ---\r\n", cycle_count + 1);
        
        GUI_Set_Active_Page(PAGE_DESKTOP);
        GUI_Draw_Active_Page();
        GUI_Process_Touch();
        BSP_DWT_Delay_ms(2000);
        
        GUI_Set_Active_Page(PAGE_OSCILLOSCOPE);
        GUI_Draw_Active_Page();
        GUI_Process_Touch();
        BSP_DWT_Delay_ms(2000);
        
        GUI_Set_Active_Page(PAGE_SIGNAL_GEN);
        GUI_Draw_Active_Page();
        GUI_Process_Touch();
        BSP_DWT_Delay_ms(2000);
    }
    
    Test_Printf("\r\n");
    Test_Printf("========================================\r\n");
    Test_Printf("=== Test Statistics ===\r\n");
    Test_Printf("Total Cycles Completed: %lu\r\n", cycle_count);
    Test_Printf("Total Button Clicks: %lu\r\n", g_button_click_count);
    Test_Printf("Status: GUI framework operational\r\n");
    Test_Printf("========================================\r\n");
}

/* ==================== 测试启动器 ==================== */

/**
 * @brief 统一的测试启动入口
 * 
 * 替代了原来的 Test_Start() 和 Test_Selector_Run()
 * 所有测试模块通过此函数统一启动
 */
void Test_Start(void)
{
    Test_Printf("\r\n");
    Test_Printf("------------------------------------------\r\n");
    Test_Printf("-   Test Framework Started               -\r\n");
    Test_Printf("-   Current Mode: %d                     -\r\n", TEST_CURRENT_MODE);
    Test_Printf("------------------------------------------\r\n");
    
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
    Test_Printf("------------------------------------------\r\n");
    Test_Printf("-   All Tests Completed                  -\r\n");
    Test_Printf("------------------------------------------\r\n");
    Test_Printf("\r\n");
}
