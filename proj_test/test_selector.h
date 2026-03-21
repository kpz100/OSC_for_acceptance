#ifndef TEST_SELECTOR_H
#define TEST_SELECTOR_H

/**
 * @file test_selector.h
 * @brief 测试选择器 - 已弃用，保留向后兼容
 * 
 * @deprecated 此文件已弃用
 * 
 * 新的使用方式:
 *   #include "test_utils.h"
 *   
 *   // 在 test_utils.h 中修改 TEST_CURRENT_MODE
 *   // 然后调用
 *   Test_Start();
 * 
 * 新的配置宏在 test_utils.h 中:
 *   - TEST_MODE_ORIGINAL_LCD
 *   - TEST_MODE_ORIGINAL_TOUCH
 *   - TEST_MODE_LCD_CONTROL_INIT
 *   - TEST_MODE_LCD_CONTROL_BTN
 *   - TEST_MODE_LCD_CONTROL_PAGE
 *   - TEST_MODE_LCD_CONTROL_FULL
 */

#include "test_utils.h"

/** @deprecated 使用 Test_Start() 替代 */
#define Test_Selector_Run() Test_Start()

#endif
