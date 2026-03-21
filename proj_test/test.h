#ifndef TEST_H
#define TEST_H

/**
 * @file test.h
 * @brief 原始LCD和触摸屏测试模块
 * 
 * 注意: 此头文件保持向后兼容
 * 建议使用 test_utils.h 中的 Test_Start() 替代此模块
 * 
 * 新的统一入口:
 *   #include "test_utils.h"
 *   Test_Start();  // 自动选择TEST_CURRENT_MODE对应的测试
 */

#include "test_utils.h"  // 使用统一的测试工具库

/**
 * @brief 原始LCD综合测试
 * @deprecated 使用 Test_Start() 替代
 */
void Test_LCD_Comprehensive(void);

/**
 * @brief 原始触摸屏综合测试
 * @deprecated 使用 Test_Start() 替代
 */
void Test_Touch_Comprehensive(void);

#endif