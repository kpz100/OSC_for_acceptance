#include "lcd_control_ui.h"
#include <string.h>

// 实例化内存池
LCD_UI_Pool_Struct g_ui_pool __attribute__((section(".bss.ARM.__at_0xC0400000"))) __ALIGNED(32);

void LCD_UI_Pool_Init(void) {
    memset(&g_ui_pool, 0, sizeof(LCD_UI_Pool_Struct));
}

// 修正：之前误用了 LCD_Button_Struct
LCD_TXT_Struct* LCD_UI_CreateTXT(const char* inner_name, uint16_t x, uint16_t y, uint16_t w, uint16_t h, 
                                 uint32_t bg_color, const char* txt, uint8_t font_type, uint32_t font_color) {
    if (g_ui_pool.txt_count >= MAX_TEXT_COUNT) return NULL;

    LCD_TXT_Struct* p_txt = &g_ui_pool.texts[g_ui_pool.txt_count++];
    
    p_txt->figure.x = x;
    p_txt->figure.y = y;
    p_txt->figure.w = w;
    p_txt->figure.h = h;
    p_txt->figure.bg_color = bg_color;
    p_txt->font_type = font_type;
    p_txt->font_color = font_color;
    
    strncpy(p_txt->figure.inner_name, inner_name, MAX_INNER_NAME_LENGTH - 1);
    strncpy(p_txt->txt, txt, MAX_TXT_LENGTH - 1);

    return p_txt;
}

LCD_Button_Struct* LCD_UI_CreateButton(const char* inner_name, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t bg_color) {
    if (g_ui_pool.button_count >= MAX_BUTTON_COUNT) return NULL;

    LCD_Button_Struct* btn = &g_ui_pool.buttons[g_ui_pool.button_count++];
    
    btn->figure.x = x;
    btn->figure.y = y;
    btn->figure.w = w;
    btn->figure.h = h;
    btn->figure.bg_color = bg_color;
    btn->pressed = 0;
    
    strncpy(btn->figure.inner_name, inner_name, MAX_INNER_NAME_LENGTH - 1);
    
    return btn;
}

// 补全：波形控件初始化逻辑
LCD_Waveform_Struct* LCD_UI_CreateWaveform(const char* inner_name, uint16_t x, uint16_t y, uint16_t w, uint16_t h, 
                                           uint32_t bg_color, uint32_t line_color) {
    if (g_ui_pool.wave_count >= MAX_WAVE_COUNT) return NULL;

    LCD_Waveform_Struct* wf = &g_ui_pool.waves[g_ui_pool.wave_count++];

    wf->figure.x = x;
    wf->figure.y = y;
    wf->figure.w = w;
    wf->figure.h = h;
    wf->figure.bg_color = bg_color;
    wf->line_color = line_color;
    
    strncpy(wf->figure.inner_name, inner_name, MAX_INNER_NAME_LENGTH - 1);

    // 初始化通道数据为空
    for(uint8_t i=0; i<MAX_WAVEFORM_CHANNEL; i++) {
        wf->data[i] = NULL;
        wf->length[i] = 0;
    }

    return wf;
}