#include "test.h"
#include "usart.h"
#include <stdio.h>
#include <stdarg.h>

/**
 * @brief Print test message to USART
 */
static void Test_Printf(const char *format, ...)
{
    // Implementation depends on available USART printf function
    // This is a placeholder for debug output
	
	char buffer[256]; // 根据需要调整缓冲区大小
    va_list args;
    
    va_start(args, format);
    // 使用 vsnprintf 安全地格式化字符串
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (len > 0) {
        // 使用 HAL 库阻塞式发送，假设你使用的是 huart1
        // 这里的 100 是超时时间（ms）
        HAL_UART_Transmit(&huart1, (uint8_t *)buffer, len, 100);
    }
}

/**
 * @brief Test LCD color display
 * Display all color definitions on screen
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
    BSP_DWT_Delay_ms(2000);  // Display for 2 seconds
}

/**
 * @brief Test LCD drawing pixels
 */
static void Test_LCD_Pixels(void)
{
    Test_Printf("Testing LCD Pixel Drawing...\r\n");
    
    BSP_LCD_Clear(LCD_COLOR_BLACK);
    
    // Draw diagonal lines
    for (int i = 0; i < 400; i++)
    {
        BSP_LCD_DrawPixel(100 + i / 2, 100 + i, LCD_COLOR_GREEN);
    }
    
    // Draw vertical line
    for (int i = 0; i < 100; i++)
    {
        BSP_LCD_DrawPixel(400, 200 + i, LCD_COLOR_BLUE);
    }
    
    // Draw horizontal line
    for (int i = 0; i < 150; i++)
    {
        BSP_LCD_DrawPixel(250 + i, 350, LCD_COLOR_RED);
    }
    
    BSP_LCD_Flip();
    BSP_DWT_Delay_ms(2000);
}

/**
 * @brief Test LCD gradient effect
 */
static void Test_LCD_Gradient(void)
{
    Test_Printf("Testing LCD Gradient...\r\n");
    
    BSP_LCD_Clear(LCD_COLOR_BLACK);
    
    // Red gradient
    for (int x = 0; x < 200; x++)
    {
        uint8_t red = (x * 255) / 200;
        uint32_t color = 0xFF000000 | (red << 16);
        BSP_LCD_FillRect(x, 0, 1, LCD_HEIGHT, color);
    }
    
    // Green gradient
    for (int x = 200; x < 400; x++)
    {
        uint8_t green = ((x - 200) * 255) / 200;
        uint32_t color = 0xFF000000 | (green << 8);
        BSP_LCD_FillRect(x, 0, 1, LCD_HEIGHT, color);
    }
    
    // Blue gradient
    for (int x = 400; x < 600; x++)
    {
        uint8_t blue = ((x - 400) * 255) / 200;
        uint32_t color = 0xFF000000 | blue;
        BSP_LCD_FillRect(x, 0, 1, LCD_HEIGHT, color);
    }
    
    // White gradient
    for (int x = 600; x < LCD_WIDTH; x++)
    {
        uint8_t gray = ((x - 600) * 255) / (LCD_WIDTH - 600);
        uint32_t color = 0xFF000000 | (gray << 16) | (gray << 8) | gray;
        BSP_LCD_FillRect(x, 0, 1, LCD_HEIGHT, color);
    }
    
    BSP_LCD_Flip();
    BSP_DWT_Delay_ms(2000);
}

/**
 * @brief Test LCD checkerboard pattern
 */
static void Test_LCD_Checkerboard(void)
{
    Test_Printf("Testing LCD Checkerboard Pattern...\r\n");
    
    BSP_LCD_Clear(LCD_COLOR_BLACK);
    
    uint16_t square_size = 40;
    uint32_t colors[] = {LCD_COLOR_WHITE, LCD_COLOR_BLACK};
    
    for (uint16_t y = 0; y < LCD_HEIGHT; y += square_size)
    {
        for (uint16_t x = 0; x < LCD_WIDTH; x += square_size)
        {
            uint32_t color_index = ((x / square_size) + (y / square_size)) % 2;
            BSP_LCD_FillRect(x, y, square_size, square_size, colors[color_index]);
        }
    }
    
    BSP_LCD_Flip();
    BSP_DWT_Delay_ms(2000);
}

/**
 * @brief Test LCD fill operations
 */
static void Test_LCD_Fills(void)
{
    Test_Printf("Testing LCD Fill Operations...\r\n");
    
    // Fill entire screen with different colors
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
 * @brief Test LCD buffer flipping
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

#include "tim.h"

/**
 * @brief Comprehensive LCD test - executes all test cases
 */
void Test_LCD_Comprehensive(void)
{
    Test_Printf("\r\n=== Starting Comprehensive LCD Test ===\r\n");
    
    // Initialize LCD first
    HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_1);
    BSP_DWT_Init();
    BSP_SDRAM_Init();
    BSP_LCD_Init();
    BSP_DWT_Delay_ms(500);
	
	uint32_t time = 0;
    
    for (int i = 0; i < 100; i++) {
		// Test 1: Color display
		Test_LCD_Colors();
		
		// Test 2: Pixel drawing
		Test_LCD_Pixels();
		
		// Test 3: Gradient effects
		Test_LCD_Gradient();
		
		// Test 4: Checkerboard pattern
		Test_LCD_Checkerboard();
		
		// Test 5: Fill operations
		Test_LCD_Fills();
		
		// Test 6: Buffer flipping
		Test_LCD_Flipping();
		
		// Final: Complete test successful
		BSP_LCD_Clear(LCD_COLOR_BLACK);
		BSP_LCD_FillRect(250, 200, 300, 80, LCD_COLOR_GREEN);
		BSP_LCD_Flip();
		
		Test_Printf("=== The %lu Time OK ===\r\n\r\n", time);
		Test_Printf("=== Comprehensive LCD Test Completed Successfully ===\r\n\r\n");
		time++;
		BSP_DWT_Delay_ms(1000);
	}
}
