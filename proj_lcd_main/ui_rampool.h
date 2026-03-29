#ifndef UI_RAMPOOL_H
#define UI_RAMPOOL_H

#include "main.h"
#include "ui_type.h"

#define MAX_BUTTON_COUNT        50u
#define MAX_TEXT_COUNT          50u
#define MAX_WAVE_COUNT          10u

typedef struct {
    uint8_t button_count;
    uint8_t txt_count;
    uint8_t wave_count;

    LCD_Button_Struct   buttons[MAX_BUTTON_COUNT];
    LCD_TXT_Struct      texts[MAX_TEXT_COUNT];
    LCD_Waveform_Struct waves[MAX_WAVE_COUNT];
} LCD_UI_Pool_Struct;

extern LCD_UI_Pool_Struct g_ui_pool __attribute__((section(".bss.ARM.__at_0xC0400000"))) __ALIGNED(32); // mpu关闭sdram的所有缓存

void LCD_UI_Pool_Init(void);
LCD_TXT_Struct* LCD_UI_CreateTXT(const char* inner_name, uint16_t x, uint16_t y, uint16_t w, uint16_t h, 
                                 uint32_t bg_color, const char* txt, uint8_t font_type, uint32_t font_color);
LCD_Button_Struct* LCD_UI_CreateButton(const char* inner_name, uint16_t x, uint16_t y, uint16_t w, uint16_t h, 
                                        uint32_t bg_color, const char* txt, uint8_t font_type, uint32_t font_color);
LCD_Waveform_Struct* LCD_UI_CreateWaveform(const char* inner_name, uint16_t x, uint16_t y, uint16_t w, uint16_t h, 
                                           uint32_t bg_color, uint32_t line_color, uint32_t waveform_color0, uint32_t waveform_color1);
void LCD_UI_ClearPool(void);

#endif
