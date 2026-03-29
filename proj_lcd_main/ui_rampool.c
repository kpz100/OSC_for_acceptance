#include "ui_rampool.h"
#include "ui_type.h"
#include <string.h>

LCD_UI_Pool_Struct g_ui_pool __attribute__((section(".bss.ARM.__at_0xC0400000"))) __ALIGNED(32);

void LCD_UI_Pool_Init(void) {
    memset(&g_ui_pool, 0, sizeof(LCD_UI_Pool_Struct));
}

LCD_TXT_Struct* LCD_UI_CreateTXT(const char* inner_name, uint16_t x, uint16_t y, uint16_t w, uint16_t h, 
                                 uint32_t bg_color, const char* txt, uint8_t font_type, uint32_t font_color) {
    if (g_ui_pool.txt_count >= MAX_TEXT_COUNT) return NULL;

    LCD_TXT_Struct* p_txt = &g_ui_pool.texts[g_ui_pool.txt_count++];
    
    p_txt->figure.x = x;
    p_txt->figure.y = y;
    p_txt->figure.w = w;
    p_txt->figure.h = h;
    p_txt->figure.bg_color = bg_color;
    p_txt->figure.ui_type = UI_TXT;
    p_txt->font_type = font_type;
    p_txt->font_color = font_color;
    
    strncpy(p_txt->figure.inner_name, inner_name, MAX_INNER_NAME_LENGTH - 1);
    p_txt->figure.inner_name[MAX_INNER_NAME_LENGTH - 1] = '\0';
    strncpy(p_txt->txt, txt, MAX_TXT_LENGTH - 1);
    p_txt->txt[MAX_TXT_LENGTH - 1] = '\0';

    p_txt->refresh_txt = NULL;

    return p_txt;
}

LCD_Button_Struct* LCD_UI_CreateButton(const char* inner_name, uint16_t x, uint16_t y, uint16_t w, uint16_t h, 
                                        uint32_t bg_color, const char* txt, uint8_t font_type, uint32_t font_color) {
    if (g_ui_pool.button_count >= MAX_BUTTON_COUNT) return NULL;

    LCD_Button_Struct* btn = &g_ui_pool.buttons[g_ui_pool.button_count++];
    
    btn->figure.x = x;
    btn->figure.y = y;
    btn->figure.w = w;
    btn->figure.h = h;
    btn->figure.bg_color = bg_color;
    btn->figure.ui_type = UI_BUTTON;
    btn->font_type = font_type;
    btn->font_color = font_color;
    btn->clicked = 0;
    btn->click_time = 0;
    
    strncpy(btn->figure.inner_name, inner_name, MAX_INNER_NAME_LENGTH - 1);
    btn->figure.inner_name[MAX_INNER_NAME_LENGTH - 1] = '\0';
    strncpy(btn->txt, txt, MAX_TXT_LENGTH - 1);
    btn->txt[MAX_TXT_LENGTH - 1] = '\0';
    
    btn->on_click = NULL;

    return btn;
}

// 补全：波形控件初始化逻辑
LCD_Waveform_Struct* LCD_UI_CreateWaveform(const char* inner_name, uint16_t x, uint16_t y, uint16_t w, uint16_t h, 
                                           uint32_t bg_color, uint32_t line_color, uint32_t waveform_color0, uint32_t waveform_color1) {
    if (g_ui_pool.wave_count >= MAX_WAVE_COUNT) return NULL;

    LCD_Waveform_Struct* wf = &g_ui_pool.waves[g_ui_pool.wave_count++];

    wf->figure.x = x;
    wf->figure.y = y;
    wf->figure.w = w;
    wf->figure.h = h;
    wf->figure.bg_color = bg_color;
    wf->figure.ui_type = UI_WAVEFORM;
    wf->line_color = line_color;
    wf->waveform_color[0] = waveform_color0;
	wf->waveform_color[1] = waveform_color1;										   
    
    strncpy(wf->figure.inner_name, inner_name, MAX_INNER_NAME_LENGTH - 1);
    wf->figure.inner_name[MAX_INNER_NAME_LENGTH - 1] = '\0';

    wf->drawin_buffer = NULL;
    wf->drawto_lcd = NULL;

    return wf;
}

void LCD_UI_ClearPool(void) {
    LCD_UI_Pool_Init();
}


