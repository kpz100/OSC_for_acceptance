#ifndef LCD_CONTROL_H
#define LCD_CONTROL_H

#include "main.h"

/**
 * @defgroup GUI_INTERFACE LCD控制接口
 * @brief 提供完整的GUI控制、页面管理、控件创建和事件处理功能
 * @{
 */

/* ============ 内存配置宏定义 ============ */
#define MAX_TXT_LEN 30u                    /**< 文本最大长度 */
#define MAX_WAVEFORM_CHANNELS 2u           /**< 波形显示最大通道数 */

/** 
 * @brief GUI内存池配置
 * 使用SDRAM的第二个2MB区间用作GUI动态内存分配池
 * SDRAM布局: [0xC0000000--0xC0200000) 系统用, [0xC0200000--0xC0A00000) GUI用
 */
#define GUI_MEM_POOL_ADDR  0xC0200000     /**< GUI内存池起始地址 */
#define GUI_MEM_POOL_SIZE  (10 * 1024 * 1024) /**< GUI内存池大小: 10MB */

/* ============ 页面定义 ============ */
/**
 * @enum Page_ID_t
 * @brief GUI页面类型枚举
 */
typedef enum {
    PAGE_DESKTOP = 0,       /**< 主界面 */
    PAGE_OSCILLOSCOPE,      /**< 示波器界面 */
    PAGE_SIGNAL_GEN,        /**< 信号发生器界面 */
    PAGE_MAX                /**< 页面总数 */
} Page_ID_t;

/* ============ 控件结构体定义 ============ */

/**
 * @struct LCD_TXT_Struct
 * @brief 文本显示控件结构体
 */
typedef struct {
    uint16_t x;             /**< X坐标 */
    uint16_t y;             /**< Y坐标 */
    uint8_t font_type;      /**< 字体类型 */
    uint32_t color;         /**< 文本颜色 */
    char text[MAX_TXT_LEN]; /**< 文本内容 */
} LCD_TXT_Struct;

/**
 * @struct LCD_Button_Struct
 * @brief 按钮控件结构体
 * 
 * 按钮支持按下/弹起状态切换和点击回调
 */
typedef struct {
    uint16_t x;             /**< 左上角X坐标 */
    uint16_t y;             /**< 左上角Y坐标 */
    uint16_t w;             /**< 按钮宽度 */
    uint16_t h;             /**< 按钮高度 */
    uint8_t font_type;      /**< 字体类型 */
    uint32_t color;         /**< 文本颜色 */
    uint32_t bg_color;      /**< 背景颜色 */
    char text[MAX_TXT_LEN]; /**< 按钮文本 */
    uint8_t state;          /**< 状态标志: 0-弹起, 1-按下 */
    void (*OnClick)(void);  /**< 点击回调函数指针 */
} LCD_Button_Struct;

/**
 * @struct LCD_Waveform_Struct
 * @brief 波形显示控件结构体
 * 
 * 用于显示多通道波形数据(如示波器、信号显示等)
 */
typedef struct {
    uint16_t x;                              /**< 左上角X坐标 */
    uint16_t y;                              /**< 左上角Y坐标 */
    uint16_t w;                              /**< 显示区域宽度 */
    uint16_t h;                              /**< 显示区域高度 */
    uint8_t * data[MAX_WAVEFORM_CHANNELS];   /**< 各通道数据缓冲区指针 */
    uint32_t data_color[MAX_WAVEFORM_CHANNELS]; /**< 各通道显示颜色 */
    uint32_t bg_color;                       /**< 背景颜色 */
    uint32_t line_color;                     /**< 网格线颜色 */
} LCD_Waveform_Struct;

/**
 * @struct LCD_Page_Struct
 * @brief GUI页面结构体
 * 
 * 管理一个完整页面的所有控件(文本、按钮、波形等)
 */
typedef struct {
    Page_ID_t id;                    /**< 页面ID */
    
    LCD_TXT_Struct** texts;          /**< 文本控件指针数组 */
    uint16_t text_count;             /**< 文本控件数量 */

    LCD_Button_Struct** buttons;     /**< 按钮控件指针数组 */
    uint16_t button_count;           /**< 按钮控件数量 */

    LCD_Waveform_Struct** waveforms; /**< 波形控件指针数组 */
    uint16_t waveform_count;         /**< 波形控件数量 */
} LCD_Page_Struct;

/* ============ 核心接口函数 ============ */

/**
 * @brief LCD和SDRAM初始化
 * 
 * 初始化流程:
 *   1. 启动PWM背光
 *   2. 初始化DWT计时器
 *   3. 初始化SDRAM
 *   4. 初始化LCD屏幕
 *   5. 配置触摸屏I2C接口
 *   6. 执行GT911触摸屏复位序列
 *   7. 初始化所有页面结构
 */
void LCD_SDRAM_DWT_Init(void);

/**
 * @brief 设置当前活跃页面
 * @param page_id 目标页面ID
 * 
 * 切换页面时会设置重绘标志位，下次调用GUI_Draw_Active_Page()时进行整体重绘
 */
void GUI_Set_Active_Page(Page_ID_t page_id);

/**
 * @brief 绘制当前活跃页面
 * 
 * 绘制策略:
 *   - 首次进入页面: 执行整体清屏+重绘
 *   - 后续更新: 根据页面类型进行局部或全局更新
 * 
 * 应该在主循环中定期调用此函数
 */
void GUI_Draw_Active_Page(void);

/**
 * @brief 处理触摸屏输入
 * 
 * 功能:
 *   - 扫描触摸屏
 *   - 进行按钮的碰撞检测
 *   - 触发按钮的点击回调
 *   - 更新按钮的视觉状态
 * 
 * 应该在主循环中定期调用此函数
 */
void GUI_Process_Touch(void);

/* ============ 控件创建函数 ============ */

/**
 * @brief 创建按钮控件
 * @param page_id   所属页面ID
 * @param x         左上角X坐标
 * @param y         左上角Y坐标
 * @param w         按钮宽度(像素)
 * @param h         按钮高度(像素)
 * @param text      按钮文本
 * @param OnClick   点击回调函数指针
 * @return 按钮控件指针，内存分配失败时返回NULL
 * 
 * 注意: 按钮内存分配自SDRAM的GUI内存池，需要确保内存充足
 */
LCD_Button_Struct* GUI_Create_Button(Page_ID_t page_id, uint16_t x, uint16_t y, uint16_t w, uint16_t h, char* text, void (*OnClick)(void));

/**
 * @brief 创建波形显示控件
 * @param page_id   所属页面ID
 * @param x         左上角X坐标
 * @param y         左上角Y坐标
 * @param w         显示区域宽度(像素)
 * @param h         显示区域高度(像素)
 * @return 波形控件指针，内存分配失败时返回NULL
 * 
 * 创建后需要通过GUI_Create_Button返回的指针设置data[]指向实际的ADC缓冲区
 */
LCD_Waveform_Struct* GUI_Create_Waveform(Page_ID_t page_id, uint16_t x, uint16_t y, uint16_t w, uint16_t h);

/* ============ 控件绘制函数 ============ */

/**
 * @brief 绘制单个按钮控件
 * @param btn 按钮指针
 * 
 * 内部调用，用于按钮状态的视觉反馈更新
 */
void Draw_Button(LCD_Button_Struct* btn);

/**
 * @brief 绘制波形显示控件
 * @param wf 波形指针
 * 
 * 内部调用，用于波形数据的实时显示
 */
void Draw_Waveform(LCD_Waveform_Struct* wf);

/** @} */

#endif