#ifndef TEST_ALL_H
#define TEST_ALL_H

/**
 * @file test_all.h
 * @brief 统一的测试模块头文件 - 完整独立的测试框架
 * 
 * 包含所有硬件驱动测试和GUI框架测试的函数声明
 * 此文件是自包含的，不依赖其他测试模块头文件
 * 
 * 使用示例:
 *   #include "test_all.h"
 *   Test_Start();  // 根据TEST_CURRENT_MODE运行相应的测试
 * 
 * 配置测试模式:
 *   在 test_all.c 中修改 TEST_CURRENT_MODE 宏
 */

#include "main.h"
#include <stdarg.h>

/* ============ 调试输出 ============ */

/**
 * @brief 统一的测试调试输出函数
 * @param format 格式字符串 (printf风格)
 * @param ... 可变参数
 * 
 * 所有测试模块都应使用此函数输出调试信息
 * 通过USART1输出，波特率115200 bps
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
 */
void Test_Print_Title(const char *title);

/**
 * @brief 打印测试分隔符
 * @param level 分隔符级别 (1-主级, 2-次级)
 */
void Test_Print_Separator(uint8_t level);

/* ============ 测试启动入口 ============ */

/**
 * @brief 启动测试框架
 * 
 * 根据 TEST_CURRENT_MODE 宏执行对应的测试
 * 在 main.c 中调用此函数开始测试
 * 
 * 调用示例:
 *   Test_Start();  // 在main()中调用
 */
void Test_Start(void);

/* ============ LCD和触摸屏驱动测试 ============ */

/**
 * @brief LCD驱动综合测试
 * 
 * 测试内容:
 *   - LCD颜色显示
 *   - 像素绘制
 *   - 梯度效果
 *   - 棋盘图案
 *   - 填充操作
 *   - 缓冲翻转
 * 
 * 运行模式: TEST_MODE_ORIGINAL_LCD
 */
void Test_LCD_Comprehensive(void);

/**
 * @brief 触摸屏驱动综合测试
 * 
 * 测试内容:
 *   - GT911初始化
 *   - 触摸扫描
 *   - I2C通信
 *   - 多点触摸
 *   - 连续轮询
 * 
 * 运行模式: TEST_MODE_ORIGINAL_TOUCH
 */
void Test_Touch_Comprehensive(void);

/* ============ LCD控制框架测试 ============ */

/**
 * @brief LCD控制框架初始化测试
 * 
 * 验证内容:
 *   - LCD_SDRAM_DWT_Init()初始化
 *   - 页面结构体初始化
 *   - 内存分配器状态
 * 
 * 运行模式: TEST_MODE_LCD_CONTROL_INIT
 */
void Test_LCD_Control_Init(void);

/**
 * @brief 按钮控件创建和交互测试
 * 
 * 验证内容:
 *   - 按钮创建
 *   - 按钮属性设置
 *   - 点击回调执行
 *   - 视觉状态转换
 * 
 * 运行模式: TEST_MODE_LCD_CONTROL_BTN
 */
void Test_LCD_Control_Button(void);

/**
 * @brief 页面切换和管理测试
 * 
 * 验证内容:
 *   - 多页面管理
 *   - 页面间切换
 *   - 重绘标志位机制
 *   - 页面隔离
 * 
 * 运行模式: TEST_MODE_LCD_CONTROL_PAGE
 */
void Test_LCD_Control_Pages(void);

/**
 * @brief 完整的GUI集成测试
 * 
 * 验证内容:
 *   - 系统初始化
 *   - 多页面创建
 *   - 页面循环展示
 *   - 按钮交互
 *   - 长时间稳定性
 * 
 * 运行模式: TEST_MODE_LCD_CONTROL_FULL
 */
void Test_LCD_Control_Full(void);

#endif
