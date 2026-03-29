#ifndef UI_TYPE_H
#define UI_TYPE_H

#include "main.h"

struct LCD_TXT_Struct;
typedef void (*TXTMethod)(struct LCD_TXT_Struct* self);
struct LCD_Button_Struct;
typedef void (*ButtonMethod)(struct LCD_Button_Struct* self);
struct LCD_Waveform_Struct;
typedef void (*WaveformMethod)(struct LCD_Waveform_Struct* self, void* buffer, size_t _type, uint32_t true_maxval, uint32_t length, uint32_t color);
typedef void (*WavedrawMehtod)(struct LCD_Waveform_Struct* self);

#define MAX_OSC_WIDTH           800u
#define MAX_OSC_HEIGHT          255u

#define FONT_OFFSET_X           5u
#define FONT_OFFSET_Y           5u

#define MAX_WAVEFORM_CHANNEL    2u
#define MAX_TXT_LENGTH          50u
#define MAX_INNER_NAME_LENGTH   20u

typedef enum {
    UI_TXT = 0,
    UI_BUTTON,
    UI_WAVEFORM
} UI_TYPE;

typedef struct LCD_Figure_Struct{
    uint32_t bg_color;
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
    char inner_name[MAX_INNER_NAME_LENGTH];
    UI_TYPE ui_type;
} LCD_Figure_Struct;

typedef struct LCD_TXT_Struct {
    LCD_Figure_Struct figure;
    char txt[MAX_TXT_LENGTH];
    uint8_t font_type;
    uint32_t font_color;

    TXTMethod refresh_txt;
} LCD_TXT_Struct;

typedef struct LCD_Button_Struct {
    LCD_Figure_Struct figure;
    char txt[MAX_TXT_LENGTH];
    uint8_t font_type;
    uint32_t font_color;

    uint8_t clicked;
    uint32_t click_time;

    ButtonMethod on_click;
} LCD_Button_Struct;

typedef struct LCD_Waveform_Struct {
    LCD_Figure_Struct figure;
    uint32_t line_color;
    uint32_t waveform_color[MAX_WAVEFORM_CHANNEL];
    uint32_t osc_draw_buffer[MAX_OSC_WIDTH * MAX_OSC_HEIGHT];

    WaveformMethod drawin_buffer;
    WavedrawMehtod drawto_lcd;
} LCD_Waveform_Struct;

#endif