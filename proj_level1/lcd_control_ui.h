#ifndef LCD_CONTROL_UI_H
#define LCD_CONTROL_UI_H

#include "main.h"

#define MAX_WAVEFORM_CHANNEL    2u
#define MAX_TXT_LENGTH          40u
#define MAX_INNER_NAME_LENGTH   20u

// 内存池容量
#define MAX_BUTTON_COUNT        50u
#define MAX_TEXT_COUNT          50u
#define MAX_WAVE_COUNT          6u

typedef struct {
    uint32_t bg_color;
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
    char inner_name[MAX_INNER_NAME_LENGTH];
} LCD_Figure_Struct;

typedef struct {
    LCD_Figure_Struct figure;
    char txt[MAX_TXT_LENGTH];
    uint8_t font_type;
    uint32_t font_color;
} LCD_TXT_Struct;

typedef struct {
    LCD_Figure_Struct figure;
    uint8_t pressed;
} LCD_Button_Struct;

typedef struct {
    LCD_Figure_Struct figure;
    uint32_t line_color;
    uint32_t* data[MAX_WAVEFORM_CHANNEL];
    uint32_t length[MAX_WAVEFORM_CHANNEL];
    uint32_t waveform_color[MAX_WAVEFORM_CHANNEL];
} LCD_Waveform_Struct;

typedef struct {
    uint8_t button_count;
    uint8_t txt_count;
    uint8_t wave_count;

    LCD_Button_Struct   buttons[MAX_BUTTON_COUNT];
    LCD_TXT_Struct      texts[MAX_TEXT_COUNT];
    LCD_Waveform_Struct waves[MAX_WAVE_COUNT];
} LCD_UI_Pool_Struct;

// 导出全局单例，强制定位到 SDRAM 指定地址
extern LCD_UI_Pool_Struct g_ui_pool __attribute__((section(".bss.ARM.__at_0xC0400000"))) __ALIGNED(32); // 不要修改keil指定的编译方式

// 函数声明
void LCD_UI_Pool_Init(void);
LCD_TXT_Struct* LCD_UI_CreateTXT(const char* name, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t bg, const char* txt, uint8_t font, uint32_t color);
LCD_Button_Struct* LCD_UI_CreateButton(const char* name, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t bg);
LCD_Waveform_Struct* LCD_UI_CreateWaveform(const char* name, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t bg, uint32_t line_color);

#endif