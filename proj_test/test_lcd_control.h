#ifndef TEST_LCD_CONTROL_H
#define TEST_LCD_CONTROL_H

#include "main.h"
#include "lcd_control.h"
#include "bsp_lcd_single.h"
#include "bsp_dwt.h"
#include "test_utils.h"

/**
 * @defgroup LCD_CONTROL_TEST LCD控制模块测试
 * @brief 测试LCD GUI框架的各项功能
 * 
 * 注意: 此模块已与统一的测试框架集成
 * 
 * 使用方式:
 *   1. 在 test_utils.h 中修改 TEST_CURRENT_MODE 宏
 *   2. 调用 Test_Start() 启动测试
 * 
 * @{
 */

/* ============ 测试函数声明 ============ */

/**
 * @brief 初始化测试
 * 
 * 测试内容:
 *   - 验证LCD_SDRAM_DWT_Init()初始化成功
 *   - 检查页面结构体是否正确初始化
 *   - 验证SDRAM内存分配器状态
 */
void Test_LCD_Control_Init(void);

/**
 * @brief 按钮创建和交互测试
 * 
 * 测试内容:
 *   - 创建多个按钮控件
 *   - 验证按钮属性(位置、大小、颜色、回调)
 *   - 测试按钮视觉反馈(按下/弹起)
 *   - 模拟触摸输入，验证回调执行
 */
void Test_LCD_Control_Button(void);

/**
 * @brief 页面切换测试
 * 
 * 测试内容:
 *   - 创建多个页面的控件
 *   - 测试页面间的切换(GUI_Set_Active_Page)
 *   - 验证重绘标志位机制
 *   - 测试页面隔离(不同页面的控件互不干扰)
 */
void Test_LCD_Control_Pages(void);

/**
 * @brief 完整功能集成测试
 * 
 * 测试内容:
 *   - 初始化所有系统
 *   - 创建三个演示页面(主页、示波器、信号发生器)
 *   - 循环演示页面切换、按钮交互、显示更新
 *   - 长时间运行测试系统稳定性
 */
void Test_LCD_Control_Full(void);

/** @} */

#endif
