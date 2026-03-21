#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include "main.h"
#include <stdarg.h>

#include "usart.h"
#include "tim.h"
#include "bsp_dwt.h"
#include "bsp_sdram.h"
#include "bsp_lcd_single.h"
#include "bsp_touch.h"

/**
 * @file test_utils.h
 * @brief 测试通用工具库 - 提供所有测试模块共用的函数
 * 
 * 功能：
 *   - 统一的调试输出接口 (Test_Printf)
 *   - 通用的硬件初始化 (Test_Init_Hardware)
 *   - 测试框架配置管理
 */

/* ============ 调试输出 ============ */

/**
 * @brief 统一的测试调试输出函数
 * @param format 格式字符串 (printf风格)
 * @param ... 可变参数
 * 
 * 所有测试模块都应使用此函数输出调试信息
 * 通过USART1输出，波特率115200 bps
 * 
 * 使用示例:
 *   Test_Printf("Test %d started\r\n", test_id);
 *   Test_Printf("Value: 0x%02X\r\n", value);
 */
void Test_Printf(const char *format, ...);

/* ============ 硬件初始化 ============ */

/**
 * @brief 获取硬件初始化状态
 * @return 1: 已初始化, 0: 未初始化
 */
uint8_t Test_Is_Hardware_Initialized(void);

/**
 * @brief LCD/SDRAM/触摸屏通用初始化
 * 
 * 初始化顺序(重要):
 *   1. HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_1) - 背光
 *   2. BSP_DWT_Init() - DWT计时器
 *   3. BSP_SDRAM_Init() - SDRAM
 *   4. BSP_LCD_Init() - LCD屏幕
 *   5. Touch_I2C_GPIO_Config() - 触摸I2C
 *   6. GT911_Reset_Sequence() - 触摸屏复位
 * 
 * 返回值:
 *   - 1 (成功): 首次初始化完成
 *   - 0 (跳过): 已初始化，不重复初始化
 *   - -1 (失败): 初始化出错
 */
int Test_Init_Hardware(void);

/**
 * @brief 打印测试标题
 * @param title 标题文本
 * 
 * 打印带有装饰的标题，便于区分不同的测试段
 * 
 * 输出示例:
 *   ========================================
 *   === Test Title ===
 *   ========================================
 */
void Test_Print_Title(const char *title);

/**
 * @brief 打印测试分隔符
 * @param level 分隔符级别 (1-主级, 2-次级)
 * 
 * 用于区分测试的不同阶段
 */
void Test_Print_Separator(uint8_t level);

/* ============ 测试框架配置 ============ */

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
#define TEST_CURRENT_MODE           TEST_MODE_LCD_CONTROL_FULL

/* ============ 测试入口 ============ */

/**
 * @brief 启动测试
 * 
 * 根据 TEST_CURRENT_MODE 宏执行对应的测试
 * 在 main.c 中调用此函数开始测试
 * 
 * 调用示例:
 *   Test_Start();  // 在main()中调用
 */
void Test_Start(void);

#endif
