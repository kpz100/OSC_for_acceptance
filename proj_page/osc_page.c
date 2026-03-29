#include "osc_manager.h"
#include "ui_rampool.h"
#include "ui_render.h"
#include "ui_type.h"
#include "ui_touch.h"
#include "bsp_lcd_single.h"
#include "ascii_font.h"
#include <string.h>
#include <stdio.h>

#include "bsp_dwt.h"

#ifndef abs
#define abs(x) ((x) > 0) ? (x) : -(x)
#endif

static uint8_t osc_config = 0;
static uint32_t osc_tick = 0;

uint8_t Get_OSC_Config(void) {
    return osc_config;
}

static LCD_Waveform_Struct* wf_show_lcd = NULL; // MAX为最大宽高
static LCD_Button_Struct* btn_return_des = NULL;
static LCD_Button_Struct* btn_control_ch1 = NULL;
static LCD_Button_Struct* btn_control_ch2 = NULL;
static LCD_TXT_Struct* txt_ch1_vpp_fft  = NULL;
static LCD_TXT_Struct* txt_ch2_vpp_fft  = NULL;

static void Buffer_SetPixel(LCD_Waveform_Struct* self, uint16_t x, uint16_t y, uint32_t color) {
    // 意思是w&h不会超过MAX，而显示用数组是w*h大
    if (x >= self->figure.w || y >= self->figure.h) return;
    // 注意：这里的索引基于 MAX_OSC_WIDTH 以匹配结构体定义
    self->osc_draw_buffer[y * self->figure.w + x] = color;
}

static void Buffer_DrawLine(LCD_Waveform_Struct* self, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color) {
    int16_t dx = abs(x2 - x1);
    int16_t dy = abs(y2 - y1);
    int16_t sx = (x1 < x2) ? 1 : -1;
    int16_t sy = (y1 < y2) ? 1 : -1;
    int16_t err = dx - dy;
    int16_t e2;

    while (1) {
        Buffer_SetPixel(self, x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 += sy; }
    }
}

/* ---------------- 波形对象方法实现 ---------------- */

static void DrawIn_Buffer(LCD_Waveform_Struct* self, void* buffer, size_t _type, uint32_t true_maxval, uint32_t length, uint32_t color) {
    if (!self || !buffer || length == 0 || true_maxval == 0) return;

    float x_ratio = (float)length / self->figure.w;
    if (x_ratio < 1.0f) x_ratio = 1.0f;

    uint16_t last_x = 0;
    uint16_t last_y = 0;
    uint8_t first_point = 1;

    for (uint16_t x = 0; x < self->figure.w; x++) {
        uint32_t data_idx = (uint32_t)(x * x_ratio);
        if (data_idx >= length) break;

        uint32_t raw_val = 0;
        if (_type == 1) raw_val = ((uint8_t*)buffer)[data_idx];
        else if (_type == 2) raw_val = ((uint16_t*)buffer)[data_idx];
        else if (_type == 4) raw_val = ((uint32_t*)buffer)[data_idx];

        if (raw_val > true_maxval) raw_val = true_maxval;

        // 纵向映射：翻转坐标，使 0 在波形框底部
        uint16_t screen_y = (uint16_t)((uint64_t)(true_maxval - raw_val) * (self->figure.h - 1) / true_maxval);

        if (first_point) {
            Buffer_SetPixel(self, x, screen_y, color);
            first_point = 0;
        } else {
            Buffer_DrawLine(self, last_x, last_y, x, screen_y, color);
        }
        last_x = x; last_y = screen_y;
    }
}

static void DrawTo_LCD(LCD_Waveform_Struct* self) {
    if (!self) return;
    // 使用 DMA2D 将内存 Buffer 搬运到 LCD 指定位置
    BSP_LCD_DrawRGBBlock(self->figure.x, self->figure.y, self->figure.w, self->figure.h, self->osc_draw_buffer);
}

// ------------------------------------------------------------ //

static void On_Return_Click(LCD_Button_Struct* self) {
    osc_config = 0;
}

static void On_Channel_Toggle(LCD_Button_Struct* self) {
    if (strcmp(self->figure.inner_name, "btn_ch1") == 0) {
        self->figure.bg_color = (self->figure.bg_color == LCD_COLOR_DARKGREEN) ? 0xFF333333 : LCD_COLOR_DARKGREEN;
        self->clicked = (self->clicked == 0) ? 1 : 0;
        BSP_LCD_DrawString(self->figure.x + FONT_OFFSET_X, self->figure.y + FONT_OFFSET_Y, self->txt, self->font_color, self->font_type, self->figure.bg_color);
        // TODO:按钮控制
    } else if (strcmp(self->figure.inner_name, "btn_ch2") == 0){
        self->figure.bg_color = (self->figure.bg_color == LCD_COLOR_RED) ? 0xFF333333 : LCD_COLOR_RED;
        self->clicked = (self->clicked == 0) ? 1 : 0;
        BSP_LCD_DrawString(self->figure.x + FONT_OFFSET_X, self->figure.y + FONT_OFFSET_Y, self->txt, self->font_color, self->font_type, self->figure.bg_color);
        // 同理
    }
}

// ==============================================================

static void Refresh_TXT(LCD_TXT_Struct* self) {
    // 目前文本内容是由外部直接修改的，所以这里不需要额外处理
    // 但如果未来需要动态生成文本，可以在这里实现
    BSP_LCD_DrawString(self->figure.x + FONT_OFFSET_X, self->figure.y + FONT_OFFSET_Y, self->txt, self->font_color, self->font_type, self->figure.bg_color);
}

// =================================================================

void OSC_Page_Init(void) {
    osc_config = 1;
    // memset(osc_draw_buffer, 0, sizeof(osc_draw_buffer));

    LCD_UI_ClearPool();
    BSP_LCD_Clear(LCD_COLOR_BLACK); 

    btn_return_des = LCD_UI_CreateButton("btn_return", 10, 10, 80, 40, LCD_COLOR_DARKGREEN, "ReturnDES", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
    btn_return_des->on_click = On_Return_Click;

    btn_control_ch1 = LCD_UI_CreateButton("btn_ch1", 10, 360, 60, 50, LCD_COLOR_DARKGREEN, "CH1", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
    btn_control_ch2 = LCD_UI_CreateButton("btn_ch2", 10, 420, 60, 50, LCD_COLOR_DARKGREEN, "CH2", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
    btn_control_ch1->on_click = On_Channel_Toggle;
    btn_control_ch2->on_click = On_Channel_Toggle;

    txt_ch1_vpp_fft = LCD_UI_CreateTXT("ch1_vpp_fft", 80, 360, 220, 32, LCD_COLOR_BLUE, "CH1: Vpp=0.00V Freq=0.00Hz", ASCII_FONT_TYPE_8x16, LCD_COLOR_WHITE);
    txt_ch2_vpp_fft = LCD_UI_CreateTXT("ch2_vpp_fft", 80, 420, 220, 32, LCD_COLOR_BLUE, "CH2: Vpp=0.00V Freq=0.00Hz", ASCII_FONT_TYPE_8x16, LCD_COLOR_WHITE);
    txt_ch1_vpp_fft->refresh_txt = Refresh_TXT;
    txt_ch2_vpp_fft->refresh_txt = Refresh_TXT;

    wf_show_lcd = LCD_UI_CreateWaveform("wf_show", 0, 60, MAX_OSC_WIDTH, MAX_OSC_HEIGHT, LCD_COLOR_WHITE, LCD_COLOR_DARKGREEN, LCD_COLOR_GREEN, LCD_COLOR_RED);
    wf_show_lcd->drawin_buffer = DrawIn_Buffer;
    wf_show_lcd->drawto_lcd = DrawTo_LCD;
}


// UI_Render_All没有波形绘制，只有底色绘制。具体波形绘制逻辑被移出
// 注意，按钮的文本变化是固定的且由按下回调控制的，所以不需要在这里处理文本刷新
void OSC_Page_Refresh_Data(float vpp1, float fft1, float vpp2, float fft2) {
    snprintf(txt_ch1_vpp_fft->txt, MAX_TXT_LENGTH, "CH1: Vpp=%.2fV Freq=%.2fHz", vpp1, fft1);
    snprintf(txt_ch2_vpp_fft->txt, MAX_TXT_LENGTH, "CH2: Vpp=%.2fV Freq=%.2fHz", vpp2, fft2);
    txt_ch1_vpp_fft->refresh_txt(txt_ch1_vpp_fft);
    txt_ch2_vpp_fft->refresh_txt(txt_ch2_vpp_fft);
}