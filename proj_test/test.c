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
	
	char buffer[256];
    va_list args;
    
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (len > 0) {
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
/****** Touch Test Functions ******/

/**
 * @brief Test GT911 touch sensor initialization
 */
static void Test_Touch_Init(void)
{
    Test_Printf("Testing GT911 Touch Sensor Initialization...\r\n");
    
    // Initialize I2C bus
    Touch_I2C_GPIO_Config();
    Test_Printf("I2C Bus Initialized\r\n");
    
    // Perform GT911 reset sequence to set I2C address
    GT911_Reset_Sequence();
    Test_Printf("GT911 Reset Sequence Completed\r\n");
    
    BSP_DWT_Delay_ms(100);
}

/**
 * @brief Test GT911 touch sensor single scan
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
 * @brief Test GT911 I2C communication
 */
static void Test_Touch_I2C(void)
{
    Test_Printf("Testing GT911 I2C Communication...\r\n");
    
    uint8_t test_data[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    uint8_t read_data[4] = {0};
    
    // Test write to configuration register
    GT911_WR_Reg(GT_CFG_REG, test_data, 4);
    Test_Printf("Wrote test data to configuration register\r\n");
    
    BSP_DWT_Delay_ms(10);
    
    // Test read from configuration register
    GT911_RD_Reg(GT_CFG_REG, read_data, 4);
    Test_Printf("Read data: 0x%02X, 0x%02X, 0x%02X, 0x%02X\r\n",
                read_data[0], read_data[1], read_data[2], read_data[3]);
}

/**
 * @brief Test continuous touch polling
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
        BSP_DWT_Delay_ms(50);  // Poll every 50ms
    }
    
    Test_Printf("Polling Complete: Total Scans=%lu, Touch Events=%lu\r\n", scan_count, touch_events);
}

/**
 * @brief Test touch sensor multi-touch capability
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
                Test_Printf("%d simultaneous touches detected:\r\n", max_touches);
                
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
 * @brief Comprehensive touch test - executes all touch test cases
 */
void Test_Touch_Comprehensive(void)
{
    Test_Printf("\r\n=== Starting Comprehensive Touch Test ===", "\r\n");
    
    // Initialize touch sensor
	HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_1);
    BSP_DWT_Init();
    BSP_SDRAM_Init();
    BSP_LCD_Init();
    Test_Touch_Init();
    BSP_DWT_Delay_ms(500);
    
    uint32_t test_count = 0;
    
    for (test_count = 0; test_count < 1; test_count++)
    {
        Test_Printf("\r\n--- Touch Test Iteration %lu ---\r\n", test_count + 1);
        
        // Test 1: I2C Communication
        Test_Touch_I2C();
        BSP_DWT_Delay_ms(500);
        
        // Test 2: Single touch scan
        Test_Touch_Scan();
        BSP_DWT_Delay_ms(500);
        
        // Test 3: Continuous polling (5 seconds)
        Test_Touch_Polling(5000);
        BSP_DWT_Delay_ms(500);
        
        // Test 4: Multi-touch capability test (5 seconds)
        Test_Touch_MultiTouch(5000);
        BSP_DWT_Delay_ms(1000);
    }
    
    Test_Printf("\r\n=== Comprehensive Touch Test Completed Successfully ===", "\r\n");
    Test_Printf("Total test iterations: %lu\r\n\r\n", test_count);
}
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

/****** Test Selector - Based on TEST_TYPE Macro ******/

/**
 * @brief Test entry point - Choose test type via TEST_TYPE macro
 * 
 * Macro selection:
 *   - TEST_TYPE_LCD: Run comprehensive LCD test
 *   - TEST_TYPE_TOUCH: Run comprehensive touch sensor test
 */
void Test_Start(void)
{
#if TEST_TYPE == TEST_TYPE_LCD
    Test_LCD_Comprehensive();
    
#elif TEST_TYPE == TEST_TYPE_TOUCH
    Test_Touch_Comprehensive();
    
#else
    Test_Printf("ERROR: Unknown TEST_TYPE defined!\\r\\n");
#endif
}
