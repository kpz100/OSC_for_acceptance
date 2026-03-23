#include "ui_test.h"
#include "lcd_control.h"
#include "lcd_control_ui.h"
#include "bsp_lcd_single.h"
#include "bsp_touch.h"
#include "bsp_dwt.h"

#include "ascii_font.h"
#include <stdio.h>
#include <string.h>

/* 测试数据缓冲区 - 用于波形显示 */
static uint32_t wave_data_ch0[LCD_WIDTH];
static uint32_t wave_data_ch1[LCD_WIDTH];

/* 全局 UI 控件指针 */
static LCD_TXT_Struct*      g_title_text = NULL;
static LCD_TXT_Struct*      g_info_text = NULL;
static LCD_TXT_Struct*      g_coord_text = NULL;
static LCD_Button_Struct*   g_btn_start = NULL;
static LCD_Button_Struct*   g_btn_stop = NULL;
static LCD_Button_Struct*   g_btn_clear = NULL;
static LCD_Button_Struct*   g_btn_touch_test = NULL;
static LCD_Waveform_Struct* g_waveform = NULL;

/* 测试状态标志 */
static uint8_t g_running = 0;
static uint8_t g_touch_test_mode = 0;

/* 触摸合并处理相关变量 */
#define TOUCH_SAMPLE_COUNT     10   // 合并采样点数
#define TOUCH_MIN_INTERVAL_MS  100  // 最小处理间隔(ms)

static uint32_t g_last_touch_process_time = 0;  // 上次处理时间(ms)
static uint16_t g_touch_samples_x[TOUCH_SAMPLE_COUNT] = {0};
static uint16_t g_touch_samples_y[TOUCH_SAMPLE_COUNT] = {0};
static uint8_t  g_touch_sample_index = 0;
static uint8_t  g_touch_sample_valid_count = 0;

/* 生成正弦波数据用于测试 */
static void Generate_Sine_Wave(uint32_t* buffer, uint16_t length, float amplitude, float offset, float phase)
{
    for (uint16_t i = 0; i < length; i++) {
        float angle = (2.0f * 3.14159f * i / length) + phase;
        float value = amplitude * sinf(angle) + offset;
        
        // 限制范围并转换为 RGB 颜色强度 (简单的灰度或彩色映射)
        if (value > 255) value = 255;
        if (value < 0) value = 0;
        
        // 使用 ARGB8888 格式，红色通道随值变化
        buffer[i] = (0xFF << 24) | ((uint32_t)value << 16) | (0x80 << 8) | 0x80;
    }
}

/* 生成方波数据用于测试 */
static void Generate_Square_Wave(uint32_t* buffer, uint16_t length, uint16_t high_width)
{
    for (uint16_t i = 0; i < length; i++) {
        if ((i % length) < high_width) {
            buffer[i] = 0xFFFFFF00; // 黄色
        } else {
            buffer[i] = 0xFF0080FF; // 蓝色
        }
    }
}

/* 更新波形显示 */
static void Update_Waveform_Display(void)
{
    if (g_waveform == NULL) return;
    
    // 清除波形区域 (填充背景色)
    BSP_LCD_FillRect(g_waveform->figure.x, g_waveform->figure.y,
                     g_waveform->figure.w, g_waveform->figure.h,
                     g_waveform->figure.bg_color);
    
    // 绘制网格线 (简易网格，每50像素一条线)
    uint32_t grid_color = 0xFF404040;
    for (uint16_t x = g_waveform->figure.x + 50; x < g_waveform->figure.x + g_waveform->figure.w; x += 50) {
        for (uint16_t y = g_waveform->figure.y; y < g_waveform->figure.y + g_waveform->figure.h; y++) {
            BSP_LCD_DrawPixel(x, y, grid_color);
        }
    }
    for (uint16_t y = g_waveform->figure.y + 50; y < g_waveform->figure.y + g_waveform->figure.h; y += 50) {
        for (uint16_t x = g_waveform->figure.x; x < g_waveform->figure.x + g_waveform->figure.w; x++) {
            BSP_LCD_DrawPixel(x, y, grid_color);
        }
    }
    
    // 绘制边界框
    BSP_LCD_FillRect(g_waveform->figure.x, g_waveform->figure.y,
                     g_waveform->figure.w, 1, LCD_COLOR_WHITE);
    BSP_LCD_FillRect(g_waveform->figure.x, g_waveform->figure.y + g_waveform->figure.h - 1,
                     g_waveform->figure.w, 1, LCD_COLOR_WHITE);
    BSP_LCD_FillRect(g_waveform->figure.x, g_waveform->figure.y,
                     1, g_waveform->figure.h, LCD_COLOR_WHITE);
    BSP_LCD_FillRect(g_waveform->figure.x + g_waveform->figure.w - 1, g_waveform->figure.y,
                     1, g_waveform->figure.h, LCD_COLOR_WHITE);
    
    // 绘制波形数据 (逐点绘制)
    uint16_t width = g_waveform->figure.w;
    uint16_t height = g_waveform->figure.h;
    
    if (g_waveform->data[0] != NULL && g_waveform->length[0] >= width) {
        for (uint16_t i = 0; i < width; i++) {
            uint32_t color = g_waveform->data[0][i];
            // 简化：根据颜色亮度确定Y坐标
            uint8_t brightness = (color >> 16) & 0xFF;
            uint16_t y_pos = g_waveform->figure.y + height - 1 - (brightness * height / 256);
            if (y_pos >= g_waveform->figure.y && y_pos < g_waveform->figure.y + height) {
                BSP_LCD_DrawPixel(g_waveform->figure.x + i, y_pos, g_waveform->waveform_color[0]);
            }
        }
    }
}

/* 计算触摸点的平均值 (中值滤波 + 平均) */
static void Calculate_Average_Touch(uint16_t* avg_x, uint16_t* avg_y)
{
    if (g_touch_sample_valid_count == 0) {
        *avg_x = 0;
        *avg_y = 0;
        return;
    }
    
    // 使用中值滤波去除异常值，然后取平均
    // 复制有效样本
    uint16_t valid_x[TOUCH_SAMPLE_COUNT];
    uint16_t valid_y[TOUCH_SAMPLE_COUNT];
    uint8_t valid_cnt = 0;
    
    for (uint8_t i = 0; i < TOUCH_SAMPLE_COUNT; i++) {
        if (g_touch_samples_x[i] != 0 || g_touch_samples_y[i] != 0) {
            valid_x[valid_cnt] = g_touch_samples_x[i];
            valid_y[valid_cnt] = g_touch_samples_y[i];
            valid_cnt++;
        }
    }
    
    if (valid_cnt == 0) {
        *avg_x = 0;
        *avg_y = 0;
        return;
    }
    
    // 冒泡排序 X 坐标
    for (uint8_t i = 0; i < valid_cnt - 1; i++) {
        for (uint8_t j = 0; j < valid_cnt - i - 1; j++) {
            if (valid_x[j] > valid_x[j + 1]) {
                uint16_t temp = valid_x[j];
                valid_x[j] = valid_x[j + 1];
                valid_x[j + 1] = temp;
                
                temp = valid_y[j];
                valid_y[j] = valid_y[j + 1];
                valid_y[j + 1] = temp;
            }
        }
    }
    
    // 去除最小和最大值 (如果样本数大于2)
    uint32_t sum_x = 0, sum_y = 0;
    uint8_t start_idx = 0;
    uint8_t end_idx = valid_cnt;
    
    if (valid_cnt > 2) {
        start_idx = 1;
        end_idx = valid_cnt - 1;
    }
    
    for (uint8_t i = start_idx; i < end_idx; i++) {
        sum_x += valid_x[i];
        sum_y += valid_y[i];
    }
    
    uint8_t count = end_idx - start_idx;
    if (count > 0) {
        *avg_x = sum_x / count;
        *avg_y = sum_y / count;
    } else {
        *avg_x = valid_x[0];
        *avg_y = valid_y[0];
    }
}

/* 处理合并后的触摸点 */
static void Process_Touch_Point(uint16_t x, uint16_t y)
{
    // 更新坐标显示文本
    if (g_coord_text) {
        char coord_str[32];
        sprintf(coord_str, "Touch: %3d, %3d (avg)", x, y);
        strcpy((char*)g_coord_text->txt, coord_str);
        
        // 重新绘制坐标文本区域
        BSP_LCD_FillRect(g_coord_text->figure.x, g_coord_text->figure.y,
                         g_coord_text->figure.w, g_coord_text->figure.h,
                         g_coord_text->figure.bg_color);
        BSP_LCD_DrawString(g_coord_text->figure.x + 5,
                           g_coord_text->figure.y + (g_coord_text->figure.h - 16) / 2,
                           (char*)g_coord_text->txt,
                           g_coord_text->font_color,
                           g_coord_text->font_type,
                           g_coord_text->figure.bg_color);
    }
    
    // 触摸测试模式：在触摸点显示十字标记
    if (g_touch_test_mode) {
        // 绘制十字标记
        for (int i = -5; i <= 5; i++) {
            BSP_LCD_DrawPixel(x + i, y, LCD_COLOR_RED);
            BSP_LCD_DrawPixel(x, y + i, LCD_COLOR_RED);
        }
    }
    
    // 检测按钮点击
    for (uint8_t i = 0; i < g_ui_pool.button_count; i++) {
        LCD_Button_Struct* btn = &g_ui_pool.buttons[i];
        if (x >= btn->figure.x && x <= btn->figure.x + btn->figure.w &&
            y >= btn->figure.y && y <= btn->figure.y + btn->figure.h) {
            
            // 按钮按下效果
            btn->pressed = 1;
            UI_Test_DrawAll();
            BSP_DWT_Delay_ms(100);
            btn->pressed = 0;
            
            // 处理按钮功能
            if (strcmp(btn->figure.inner_name, "btn_start") == 0) {
                g_running = 1;
                if (g_info_text) {
                    strcpy((char*)g_info_text->txt, "Status: RUNNING");
                    // 刷新信息文本
                    BSP_LCD_FillRect(g_info_text->figure.x, g_info_text->figure.y,
                                     g_info_text->figure.w, g_info_text->figure.h,
                                     g_info_text->figure.bg_color);
                    BSP_LCD_DrawString(g_info_text->figure.x + 5,
                                       g_info_text->figure.y + (g_info_text->figure.h - 16) / 2,
                                       (char*)g_info_text->txt,
                                       g_info_text->font_color,
                                       g_info_text->font_type,
                                       g_info_text->figure.bg_color);
                }
            } else if (strcmp(btn->figure.inner_name, "btn_stop") == 0) {
                g_running = 0;
                if (g_info_text) {
                    strcpy((char*)g_info_text->txt, "Status: STOPPED");
                    BSP_LCD_FillRect(g_info_text->figure.x, g_info_text->figure.y,
                                     g_info_text->figure.w, g_info_text->figure.h,
                                     g_info_text->figure.bg_color);
                    BSP_LCD_DrawString(g_info_text->figure.x + 5,
                                       g_info_text->figure.y + (g_info_text->figure.h - 16) / 2,
                                       (char*)g_info_text->txt,
                                       g_info_text->font_color,
                                       g_info_text->font_type,
                                       g_info_text->figure.bg_color);
                }
            } else if (strcmp(btn->figure.inner_name, "btn_clear") == 0) {
                // 清空波形数据
                if (g_waveform) {
                    memset(g_waveform->data[0], 0, g_waveform->length[0] * 4);
                    Update_Waveform_Display();
                }
                if (g_info_text) {
                    strcpy((char*)g_info_text->txt, "Status: CLEARED");
                    BSP_LCD_FillRect(g_info_text->figure.x, g_info_text->figure.y,
                                     g_info_text->figure.w, g_info_text->figure.h,
                                     g_info_text->figure.bg_color);
                    BSP_LCD_DrawString(g_info_text->figure.x + 5,
                                       g_info_text->figure.y + (g_info_text->figure.h - 16) / 2,
                                       (char*)g_info_text->txt,
                                       g_info_text->font_color,
                                       g_info_text->font_type,
                                       g_info_text->figure.bg_color);
                }
            } else if (strcmp(btn->figure.inner_name, "btn_touch_test") == 0) {
                g_touch_test_mode = !g_touch_test_mode;
                // 刷新所有 UI
                UI_Test_DrawAll();
            }
            
            UI_Test_DrawAll();
            break;
        }
    }
}

/* 添加触摸采样点并检查是否需要处理 */
static void Add_Touch_Sample(uint16_t x, uint16_t y)
{
    uint32_t current_time = BSP_DWT_GetCounter() / 1000;  // 转换为毫秒
    
    // 存储采样点
    g_touch_samples_x[g_touch_sample_index] = x;
    g_touch_samples_y[g_touch_sample_index] = y;
    g_touch_sample_index++;
    g_touch_sample_valid_count++;
    
    // 检查是否达到采样数量
    if (g_touch_sample_index >= TOUCH_SAMPLE_COUNT) {
        // 检查时间间隔是否满足要求
        uint32_t time_diff = 0;
        if (g_last_touch_process_time != 0) {
            time_diff = current_time - g_last_touch_process_time;
        }
        
        // 如果满足时间间隔要求，则处理这批采样点
        if (g_last_touch_process_time == 0 || time_diff >= TOUCH_MIN_INTERVAL_MS) {
            // 计算平均触摸点
            uint16_t avg_x, avg_y;
            Calculate_Average_Touch(&avg_x, &avg_y);
            
            // 只有当有有效触摸点时オ处理
            if (avg_x != 0 || avg_y != 0) {
                Process_Touch_Point(avg_x, avg_y);
            }
            
            // 更新上次处理时间
            g_last_touch_process_time = current_time;
        }
        
        // 重置采样缓冲区
        g_touch_sample_index = 0;
        g_touch_sample_valid_count = 0;
        memset(g_touch_samples_x, 0, sizeof(g_touch_samples_x));
        memset(g_touch_samples_y, 0, sizeof(g_touch_samples_y));
    }
}

/* 处理触摸事件 (采集模式) */
void UI_Test_HandleTouch(void)
{
    if (GT911_Scan() && touch_data.touch_num > 0) {
        // 获取第一个触摸点坐标
        uint16_t touch_x = touch_data.x[0];
        uint16_t touch_y = touch_data.y[0];
        
        // 添加采样点
        Add_Touch_Sample(touch_x, touch_y);
    } else {
        // 没有触摸时，如果有未处理的采样点且等待时间过长，则强制处理
        if (g_touch_sample_valid_count > 0 && g_touch_sample_valid_count < TOUCH_SAMPLE_COUNT) {
            uint32_t current_time = BSP_DWT_GetCounter() / 1000;
            uint32_t time_diff = current_time - g_last_touch_process_time;
            
            // 如果超过最小间隔的2倍时间，强制处理现有采样点
            if (g_last_touch_process_time != 0 && time_diff >= (TOUCH_MIN_INTERVAL_MS * 2)) {
                uint16_t avg_x, avg_y;
                Calculate_Average_Touch(&avg_x, &avg_y);
                
                if (avg_x != 0 || avg_y != 0) {
                    Process_Touch_Point(avg_x, avg_y);
                }
                
                // 重置采样缓冲区
                g_touch_sample_index = 0;
                g_touch_sample_valid_count = 0;
                memset(g_touch_samples_x, 0, sizeof(g_touch_samples_x));
                memset(g_touch_samples_y, 0, sizeof(g_touch_samples_y));
                g_last_touch_process_time = current_time;
            }
        }
    }
}

/* 绘制所有 UI 控件 */
void UI_Test_DrawAll(void)
{
    // 清屏
    BSP_LCD_Clear(LCD_COLOR_BLACK);
    
    // 绘制标题区域背景
    BSP_LCD_FillRect(0, 0, LCD_WIDTH, 60, 0xFF2C3E50);
    
    // 绘制底部状态栏
    BSP_LCD_FillRect(0, LCD_HEIGHT - 40, LCD_WIDTH, 40, 0xFF34495E);
    
    // 绘制所有文本框
    for (uint8_t i = 0; i < g_ui_pool.txt_count; i++) {
        LCD_TXT_Struct* txt = &g_ui_pool.texts[i];
        
        // 如果背景色不是透明，则绘制背景
        if (txt->figure.bg_color != LCD_COLOR_TRANSPARENT) {
            BSP_LCD_FillRect(txt->figure.x, txt->figure.y,
                             txt->figure.w, txt->figure.h,
                             txt->figure.bg_color);
        }
        
        // 绘制文字 (带背景色)
        BSP_LCD_DrawString(txt->figure.x + 5, 
                           txt->figure.y + (txt->figure.h - 16) / 2,
                           (char*)txt->txt, 
                           txt->font_color, 
                           txt->font_type,
                           txt->figure.bg_color);
    }
    
    // 绘制所有按钮
    for (uint8_t i = 0; i < g_ui_pool.button_count; i++) {
        LCD_Button_Struct* btn = &g_ui_pool.buttons[i];
        uint32_t color = btn->pressed ? 0xFFE67E22 : btn->figure.bg_color;
        BSP_LCD_FillRect(btn->figure.x, btn->figure.y,
                         btn->figure.w, btn->figure.h, color);
        // 绘制边框
        BSP_LCD_FillRect(btn->figure.x, btn->figure.y, btn->figure.w, 2, LCD_COLOR_WHITE);
        BSP_LCD_FillRect(btn->figure.x, btn->figure.y + btn->figure.h - 2,
                         btn->figure.w, 2, LCD_COLOR_WHITE);
        BSP_LCD_FillRect(btn->figure.x, btn->figure.y, 2, btn->figure.h, LCD_COLOR_WHITE);
        BSP_LCD_FillRect(btn->figure.x + btn->figure.w - 2, btn->figure.y,
                         2, btn->figure.h, LCD_COLOR_WHITE);
        
        // 绘制按钮文字 (根据按钮名称显示)
        char btn_text[20];
        if (strcmp(btn->figure.inner_name, "btn_start") == 0) {
            strcpy(btn_text, "START");
        } else if (strcmp(btn->figure.inner_name, "btn_stop") == 0) {
            strcpy(btn_text, "STOP");
        } else if (strcmp(btn->figure.inner_name, "btn_clear") == 0) {
            strcpy(btn_text, "CLEAR");
        } else if (strcmp(btn->figure.inner_name, "btn_touch_test") == 0) {
            strcpy(btn_text, g_touch_test_mode ? "TOUCH:ON" : "TOUCH:OFF");
        } else {
            strcpy(btn_text, btn->figure.inner_name);
        }
        
        BSP_LCD_DrawString(btn->figure.x + (btn->figure.w - strlen(btn_text) * 8) / 2,
                           btn->figure.y + (btn->figure.h - 16) / 2,
                           btn_text, 
                           LCD_COLOR_WHITE, 
                           ASCII_FONT_TYPE_8x16,
                           color);
    }
    
    // 绘制波形控件
    Update_Waveform_Display();
    
    // 更新状态栏信息 (包含采样信息)
    char status[80];
    sprintf(status, "UI Test | Touch: %d/%d samples | Interval: %dms",
            g_touch_sample_valid_count, TOUCH_SAMPLE_COUNT, TOUCH_MIN_INTERVAL_MS);
    BSP_LCD_DrawString(10, LCD_HEIGHT - 30, status, 0xFF95A5A6, 
                       ASCII_FONT_TYPE_8x16, 0xFF34495E);
}

/* 运行主测试循环 */
void UI_Test_Run(void)
{
    uint32_t frame_count = 0;
    uint32_t last_time = BSP_DWT_GetCounter();
    float phase = 0;
    
    while (1) {
        // 处理触摸事件
        UI_Test_HandleTouch();
        
        // 如果处于运行状态，更新波形数据
        if (g_running) {
            // 生成新的波形数据 (动态变化的相位)
            Generate_Sine_Wave(wave_data_ch0, LCD_WIDTH, 120.0f, 128.0f, phase);
            Generate_Square_Wave(wave_data_ch1, LCD_WIDTH, (uint16_t)(50 + 50 * sinf(phase)));
            
            // 更新波形控件数据指针
            if (g_waveform) {
                g_waveform->data[0] = wave_data_ch0;
                g_waveform->length[0] = LCD_WIDTH;
                g_waveform->data[1] = wave_data_ch1;
                g_waveform->length[1] = LCD_WIDTH;
            }
            
            // 刷新波形显示
            Update_Waveform_Display();
            
            // 相位递增
            phase += 0.05f;
            if (phase > 3.14159f * 2) phase -= 3.14159f * 2;
            
            // 更新帧率显示
            frame_count++;
            uint32_t current_time = BSP_DWT_GetCounter();
            float elapsed_ms = BSP_DWT_GetDelta_us(last_time, current_time) / 1000.0f;
            if (elapsed_ms >= 1000.0f) {
                char fps_text[32];
                sprintf(fps_text, "FPS: %.1f", frame_count * 1000.0f / elapsed_ms);
                // 绘制帧率在右下角，带半透明背景效果
                BSP_LCD_FillRect(LCD_WIDTH - 100, LCD_HEIGHT - 30, 95, 20, 0xFF34495E);
                BSP_LCD_DrawString(LCD_WIDTH - 95, LCD_HEIGHT - 28, fps_text,
                                   0xFF95A5A6, ASCII_FONT_TYPE_8x16, 0xFF34495E);
                frame_count = 0;
                last_time = current_time;
            }
        }
        
        // 简单的延时，避免CPU占用过高
        // BSP_DWT_Delay_ms(20);
    }
}

/* 初始化 UI 测试环境 */
void UI_Test_Init(void)
{
    // 初始化 SDRAM、DWT、LCD 和触摸
    LCD_SDRAM_DWT_Init();
    
    // 初始化触摸屏
    Touch_I2C_GPIO_Config();
    GT911_Reset_Sequence();
    
    // 初始化触摸采样缓冲区
    memset(g_touch_samples_x, 0, sizeof(g_touch_samples_x));
    memset(g_touch_samples_y, 0, sizeof(g_touch_samples_y));
    g_touch_sample_index = 0;
    g_touch_sample_valid_count = 0;
    g_last_touch_process_time = 0;
    
    // 初始化 UI 内存池
    LCD_UI_Pool_Init();
    
    // 创建标题文本 (背景色为标题栏颜色)
    g_title_text = LCD_UI_CreateTXT("title", 20, 10, 300, 40,
                                    0x2C3E50, "LCD UI Control Test",
                                    ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
    
    // 创建信息文本 (透明背景)
    g_info_text = LCD_UI_CreateTXT("info", 20, 70, 300, 30,
                                   LCD_COLOR_TRANSPARENT, "Status: READY",
                                   ASCII_FONT_TYPE_8x16, LCD_COLOR_CYAN);
    
    // 创建坐标显示文本 (透明背景)
    g_coord_text = LCD_UI_CreateTXT("coord", 20, 110, 300, 25,
                                    LCD_COLOR_TRANSPARENT, "Touch: ---, ---",
                                    ASCII_FONT_TYPE_8x16, LCD_COLOR_YELLOW);
    
    // 创建按钮
    g_btn_start = LCD_UI_CreateButton("btn_start", 600, 50, 80, 50, 0xFF27AE60);
    g_btn_stop = LCD_UI_CreateButton("btn_stop", 600, 120, 80, 50, 0xFFE74C3C);
    g_btn_clear = LCD_UI_CreateButton("btn_clear", 600, 190, 80, 50, 0xFFF39C12);
    g_btn_touch_test = LCD_UI_CreateButton("btn_touch_test", 600, 260, 100, 50, 0xFF3498DB);
    
    // 创建波形控件
    g_waveform = LCD_UI_CreateWaveform("wave_main", 20, 150, 550, 280,
                                       0xFF1A1A2E, LCD_COLOR_GREEN);
    
    // 设置波形通道颜色
    if (g_waveform) {
        g_waveform->waveform_color[0] = 0xFF00FF00; // 绿色
        g_waveform->waveform_color[1] = 0xFFFFA500; // 橙色
        
        // 初始化数据缓冲区
        memset(wave_data_ch0, 0, sizeof(wave_data_ch0));
        memset(wave_data_ch1, 0, sizeof(wave_data_ch1));
        
        g_waveform->data[0] = wave_data_ch0;
        g_waveform->length[0] = LCD_WIDTH;
        g_waveform->data[1] = wave_data_ch1;
        g_waveform->length[1] = LCD_WIDTH;
    }
    
    // 生成初始波形
    Generate_Sine_Wave(wave_data_ch0, LCD_WIDTH, 100.0f, 128.0f, 0);
    Generate_Square_Wave(wave_data_ch1, LCD_WIDTH, 100);
    
    // 绘制所有 UI 元素
    UI_Test_DrawAll();
}

/* 启用/关闭触摸测试模式 */
void UI_Test_EnableTouchTestMode(uint8_t enable)
{
    g_touch_test_mode = enable;
}