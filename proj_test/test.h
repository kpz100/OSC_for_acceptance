#ifndef TEST_H
#define TEST_H

#include "main.h"
#include "bsp_dwt.h"
#include "bsp_touch.h"
#include "bsp_sdram.h"
#include "bsp_lcd.h"

/****** Test Type Control Macros ******/
#define TEST_TYPE_LCD      1
#define TEST_TYPE_TOUCH    2

/* Select test type: TEST_TYPE_LCD or TEST_TYPE_TOUCH */
#define TEST_TYPE          TEST_TYPE_TOUCH

/**
 * @brief Comprehensive LCD test function
 */
void Test_LCD_Comprehensive(void);

/**
 * @brief Comprehensive touch test function
 */
void Test_Touch_Comprehensive(void);

/**
 * @brief Test entry point - Choose test type via TEST_TYPE macro in test.h
 * Call this function from main() to start testing
 */
void Test_Start(void);

#endif