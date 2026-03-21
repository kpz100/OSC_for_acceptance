#include "lcd_control.h"
#include "bsp_lcd_single.h" /**< LCD屏幕驱动 */
#include "bsp_touch.h"      /**< 触摸屏驱动 */
#include "bsp_sdram.h"      /**< SDRAM管理 */
#include "bsp_dwt.h"        /**< DWT计时器 */
#include "tim.h"            /**< 定时器(背光PWM控制) */
#include <string.h>

/* ============ 内存分配管理 ============ */

/**
 * @brief SDRAM内存分配指针
 * 
 * 从GUI_MEM_POOL_ADDR开始分配，每次分配后offset递增
 * 按4字节对齐以优化内存访问性能
 */
static uint32_t gui_mem_offset = 0;

/**
 * @brief GUI内存分配器
 * @param size 要分配的字节数
 * @return 分配到的内存地址指针，分配失败返回NULL
 * 
 * 分配策略:
 *   - 所有分配按4字节对齐
 *   - 检查可用内存是否充足
 *   - 线程不安全(单线程环境下可用)
 */
static void* GUI_Malloc(uint32_t size) {
    // 按4字节对齐计算实际分配大小
    if (gui_mem_offset + size > GUI_MEM_POOL_SIZE) {
        return NULL; // 内存不足
    }
    size = (size + 3) & ~3; 
    void* ptr = (void*)(GUI_MEM_POOL_ADDR + gui_mem_offset);
    gui_mem_offset += size;
    return ptr;
}

/* ============ 页面管理器 ============ */

/** 
 * @brief 页面数组 - 管理所有GUI页面
 * 每个页面包含该页面的所有控件(文本、按钮、波形)
 */
static LCD_Page_Struct pages[PAGE_MAX];

/**
 * @brief 当前活跃页面ID
 */
static Page_ID_t current_page = PAGE_DESKTOP;

/**
 * @brief 页面重绘标志位
 * 1: 需要整体重绘, 0: 仅需局部更新
 */
static uint8_t page_needs_redraw = 1;

/**
 * @brief LCD、SDRAM和触摸屏初始化
 * 
 * 初始化顺序(重要):
 *   1. HAL_TIM_PWM_Start - 启动背光PWM
 *   2. BSP_DWT_Init - 初始化DWT计时器(提供delay_ms等功能)
 *   3. BSP_SDRAM_Init - 初始化SDRAM
 *   4. BSP_LCD_Init - 初始化LCD屏幕
 *   5. Touch_I2C_GPIO_Config - 配置触摸屏I2C
 *   6. GT911_Reset_Sequence - 触摸屏复位(设置I2C地址)
 *   7. 初始化页面结构体
 */
void LCD_SDRAM_DWT_Init(void) {
    HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_1);
    BSP_DWT_Init();
    BSP_SDRAM_Init();
    BSP_LCD_Init();
    Touch_I2C_GPIO_Config();
    GT911_Reset_Sequence();

    // 初始化所有页面: 分配控件指针数组
    for (int i = 0; i < PAGE_MAX; i++) {
        pages[i].id = (Page_ID_t)i;
        pages[i].text_count = 0;
        pages[i].button_count = 0;
        pages[i].waveform_count = 0;
        
        // 为每个页面预分配控件指针数组空间
        // 限制: 最多20个文本/按钮, 最多5个波形
        pages[i].texts     = (LCD_TXT_Struct**)GUI_Malloc(sizeof(LCD_TXT_Struct*) * 20);
        pages[i].buttons   = (LCD_Button_Struct**)GUI_Malloc(sizeof(LCD_Button_Struct*) * 20);
        pages[i].waveforms = (LCD_Waveform_Struct**)GUI_Malloc(sizeof(LCD_Waveform_Struct*) * 5);
    }
}

/* ============ 控件创建与注册 ============ */

/**
 * @brief 创建按钮控件并注册到指定页面
 * @param page_id   目标页面ID
 * @param x         按钮左上角X坐标
 * @param y         按钮左上角Y坐标
 * @param w         按钮宽度
 * @param h         按钮高度
 * @param text      按钮显示的文本
 * @param OnClick   按钮点击时的回调函数
 * @return 创建的按钮指针，内存分配失败返回NULL
 * 
 * 功能:
 *   1. 从GUIMalloc分配内存
 *   2. 初始化按钮属性(颜色、状态等)
 *   3. 复制文本内容
 *   4. 注册到目标页面的按钮数组
 */
LCD_Button_Struct* GUI_Create_Button(Page_ID_t page_id, uint16_t x, uint16_t y, uint16_t w, uint16_t h, char* text, void (*OnClick)(void)) {
    LCD_Button_Struct* btn = (LCD_Button_Struct*)GUI_Malloc(sizeof(LCD_Button_Struct));
    if (!btn) return NULL;
    
    btn->x = x;
    btn->y = y;
    btn->w = w;
    btn->h = h;
    btn->bg_color = LCD_COLOR_BLUE;   // 默认背景色
    btn->color = LCD_COLOR_WHITE;     // 默认文字色
    btn->state = 0;                   // 初始状态: 弹起
    btn->OnClick = OnClick;
    strncpy(btn->text, text, MAX_TXT_LEN - 1);
    
    // 注册到所属页面的按钮数组
    if (pages[page_id].button_count < 20) {
        pages[page_id].buttons[pages[page_id].button_count++] = btn;
    }
    return btn;
}

// TODO: 实现其他创建函数 (GUI_Create_Text, GUI_Create_Waveform等)
// 逻辑与GUI_Create_Button完全相同，仅结构体和数组不同

/* ============ 渲染逻辑 ============ */

/**
 * @brief 绘制单个按钮
 * @param btn 按钮控件指针
 * 
 * 渲染策略:
 *   - 按钮按下时: 显示红色背景(LCD_COLOR_RED)
 *   - 按钮弹起时: 显示原始颜色(btn->bg_color)
 *   - 文本颜色始终为btn->color(需实现Draw_String函数)
 */
static void Draw_Button(LCD_Button_Struct* btn) {
    // 根据按钮状态选择背景色
    uint32_t color = (btn->state == 1) ? LCD_COLOR_RED : btn->bg_color;
    BSP_LCD_FillRect(btn->x, btn->y, btn->w, btn->h, color);
    
    // TODO: 实现文字绘制 (需要ascii_font.h支持)
    // Draw_String(btn->x + 10, btn->y + 10, btn->text, btn->color); 
}

/**
 * @brief 绘制波形显示控件
 * @param wf 波形控件指针
 * 
 * 渲染策略:
 *   1. 填充背景色
 *   2. 绘制网格线(可选)
 *   3. 遍历各通道数据，绘制波形曲线或采样点
 */
static void Draw_Waveform(LCD_Waveform_Struct* wf) {
    BSP_LCD_FillRect(wf->x, wf->y, wf->w, wf->h, wf->bg_color);
    
    // TODO: 实现波形数据绘制
    // 遍历wf->data[]绘制像素或折线图
}

/**
 * @brief 切换当前活跃页面
 * @param page_id 目标页面ID
 * 
 * 功能:
 *   - 检查页面ID有效性
 *   - 若页面变化，设置重绘标志位
 *   - 下次调用GUI_Draw_Active_Page()时进行整体重绘
 */
void GUI_Set_Active_Page(Page_ID_t page_id) {
    if (page_id < PAGE_MAX && current_page != page_id) {
        current_page = page_id;
        page_needs_redraw = 1;
    }
}

/**
 * @brief 绘制当前活跃页面
 * 
 * 绘制策略:
 *   A. 若page_needs_redraw标志位为1(页面切换或初始化):
 *      1. 清屏(LCD_COLOR_BLACK)
 *      2. 全量重绘所有控件(波形、按钮、文本)
 *      3. 清除重绘标志位
 *   
 *   B. 若page_needs_redraw标志位为0(页面已初始化):
 *      - 根据页面类型进行局部更新(如示波器页面只更新波形区域)
 *      - 减少重绘开销，提升性能和显示流畅度
 * 
 * 调用时机: 应在主循环中定期调用(如 while(1) 循环)
 */
void GUI_Draw_Active_Page(void) {
    LCD_Page_Struct* page = &pages[current_page];

    if (page_needs_redraw) {
        // 整体重绘: 清屏 + 重绘所有控件
        BSP_LCD_Clear(LCD_COLOR_BLACK);
        
        // 1. 渲染所有波形(底层)
        for(int i = 0; i < page->waveform_count; i++) {
            Draw_Waveform(page->waveforms[i]);
        }
        
        // 2. 渲染所有按钮(中层)
        for(int i = 0; i < page->button_count; i++) {
            Draw_Button(page->buttons[i]);
        }
        
        // 3. 渲染所有文本(顶层)
        // TODO: for(int i = 0; i < page->text_count; i++) Draw_Text(page->texts[i]);
        
        page_needs_redraw = 0;
    } else {
        // 局部更新: 只更新需要变化的部分
        if (current_page == PAGE_OSCILLOSCOPE) {
            // 示波器页面: 只更新波形区域(高频率更新ADC数据)
            for(int i = 0; i < page->waveform_count; i++) {
                Draw_Waveform(page->waveforms[i]);
            }
        }
        // 其他页面的局部更新策略可在这里扩展
    }
}


/* ============ 触摸屏事件处理 ============ */

/**
 * @brief 处理触摸屏输入和按钮交互
 * 
 * 功能流程:
 *   1. 扫描触摸屏(GT911_Scan)
 *   2. 若检测到触摸:
 *      - 获取触摸坐标(touch_data.x[0], touch_data.y[0])
 *      - 遍历当前页面的所有按钮
 *      - 进行碰撞检测(点是否在按钮矩形内)
 *      - 更新按钮状态(按下/弹起)
 *      - 触发回调函数(btn->OnClick)
 *      - 局部重绘改变的按钮
 *   3. 若无触摸:
 *      - 释放所有按钮(恢复弹起状态)
 *      - 恢复按钮的原始颜色
 * 
 * 注意:
 *   - 支持单点触摸，可扩展为多点
 *   - 触摸到按纽外会自动解除按钮的按下状态
 * 
 * 调用时机: 应在主循环中定期调用(如 while(1) 循环)
 */
void GUI_Process_Touch(void) {
    if (GT911_Scan()) { 
        // 触摸屏被按下
        if (touch_data.touch_num > 0) {
            uint16_t tx = touch_data.x[0];
            uint16_t ty = touch_data.y[0];
            
            LCD_Page_Struct* page = &pages[current_page];
            
            // 遍历当前页面的所有按钮进行碰撞检测
            for(int i = 0; i < page->button_count; i++) {
                LCD_Button_Struct* btn = page->buttons[i];
                
                // 检查触摸点是否在按钮矩形内
                if (tx >= btn->x && tx <= (btn->x + btn->w) &&
                    ty >= btn->y && ty <= (btn->y + btn->h)) 
                {
                    if (btn->state == 0) {
                        // 按钮从弹起变为按下
                        btn->state = 1;
                        Draw_Button(btn); // 局部重绘(变红)
                        
                        // 触发按钮的点击回调
                        if (btn->OnClick != NULL) {
                            btn->OnClick();
                        }
                    }
                } else if (btn->state == 1) {
                    // 手指在这个按钮上但现在移到了外面
                    btn->state = 0;
                    Draw_Button(btn); // 恢复原色
                }
            }
        }
    } else {
        // 触摸屏被释放或无触摸
        LCD_Page_Struct* page = &pages[current_page];
        
        // 释放所有按钮的按下状态
        for(int i = 0; i < page->button_count; i++) {
            if (page->buttons[i]->state == 1) {
                page->buttons[i]->state = 0;
                Draw_Button(page->buttons[i]); // 恢复原本颜色
            }
        }
    }
}