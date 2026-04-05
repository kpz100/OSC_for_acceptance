#include "page_manager.h"
#include "ui_rampool.h"
#include "ui_type.h"
#include "ui_touch.h"

#include "bsp_dwt.h"
#include "bsp_lcd_single.h"

#include <string.h>
#include <stdio.h>
#include "math.h"

#include "ascii_font.h"
#include "dac_control.h"
#include "tim_control.h"

#define MAX_VPP 3.3f
#define MIN_VPP 0.1f
#define VPP_STEP 0.1f

#define MAX_FREQ 50000u
#define MIN_FREQ 1000u
#define FREQ_STEP 1000u

#define MAX_DUTY 99u
#define MIN_DUTY 1u
#define DUTY_STEP 1u

#ifndef WAVE_TYPE
#define SINE_TYPE 0u
#define SQUARE_TYPE 1u
#define TRIANGLE_TYPE 2u
#endif

uint8_t gen_config = 0;

typedef struct Gen_Picture_Struct {
    LCD_Figure_Struct figure;
    uint32_t wf_color;
    uint8_t wf_type;
    uint8_t line_width;
} Gen_Picture_Struct;

static Gen_Picture_Struct gen_pic_ch1;
static Gen_Picture_Struct gen_pic_ch2;

static void Create_GPS(Gen_Picture_Struct* gps, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t bg_color, uint32_t wf_color, uint8_t wf_type, uint8_t line_width) {
    if (!gps) return;
    gps->figure.x = x;
    gps->figure.y = y;
    gps->figure.w = w;
    gps->figure.h = h;
    gps->figure.bg_color = bg_color;
    gps->wf_color = wf_color;
    gps->wf_type = wf_type;
    gps->line_width = line_width;
}

static uint16_t Get_Sinewave_Y(uint16_t x, uint16_t width, uint16_t height) {
    float ratio = (float)x / (float)(width - 1);
    float y_ratio = (sinf(ratio * 2 * 3.14159f) + 1) / 2;
    uint16_t y_pos = (uint16_t)((1.0f - y_ratio) * (height - 1));
    return y_pos;
}

static void Draw_Vertical_Line(uint16_t x, uint16_t y_start, uint16_t y_end, uint32_t color, uint8_t line_width) {
    int16_t half = line_width / 2;
    for (int16_t dx = -half; dx <= half; dx++) {
        BSP_LCD_Draw_Line(x + dx, y_start, x + dx, y_end, color);
    }
}

static void Draw_Horizontal_Line(uint16_t x_start, uint16_t x_end, uint16_t y, uint32_t color, uint8_t line_width) {
    int16_t half = line_width / 2;
    for (int16_t dy = -half; dy <= half; dy++) {
        BSP_LCD_Draw_Line(x_start, y + dy, x_end, y + dy, color);
    }
}

static void Switch_Picture_Waveform_Draw(Gen_Picture_Struct* self, uint8_t wf_type) {
    if (self && (wf_type == SINE_TYPE || wf_type == SQUARE_TYPE || wf_type == TRIANGLE_TYPE)) {
        self->wf_type = wf_type;

        uint16_t x0 = self->figure.x;
        uint16_t y0 = self->figure.y;
        uint16_t w = self->figure.w;
        uint16_t h = self->figure.h;
        uint32_t bg_color = self->figure.bg_color;
        uint32_t wave_color = self->wf_color;
        uint8_t type = self->wf_type;
        uint8_t line_width = self->line_width;
        
        if (line_width == 0) line_width = 1;

        BSP_LCD_FillRect(x0, y0, w, h, bg_color);

        if (type == SINE_TYPE) {
            for (uint16_t x = 0; x < w; x++) {
                uint16_t wave_y = Get_Sinewave_Y(x, w, h);
                Draw_Vertical_Line(x0 + x, y0 + wave_y - 1, y0 + wave_y + 1, wave_color, line_width);
            }
        } 
        else if (type == SQUARE_TYPE) {
            uint16_t mid_x = w / 2;
            Draw_Horizontal_Line(x0, x0 + mid_x, y0, wave_color, line_width);
            Draw_Vertical_Line(x0 + mid_x, y0, y0 + h - 1, wave_color, line_width);
            Draw_Horizontal_Line(x0 + mid_x, x0 + w - 1, y0 + h - 1, wave_color, line_width);
        } 
        else if (type == TRIANGLE_TYPE) {
            uint16_t mid_x = w / 2;
            int16_t half = line_width / 2;
            for (int16_t offset = -half; offset <= half; offset++) {
                BSP_LCD_Draw_Line(x0 + offset, y0 + h - 1 + offset, x0 + mid_x + offset, y0 + offset, wave_color);
                BSP_LCD_Draw_Line(x0 + mid_x + offset, y0 + offset, x0 + w - 1 + offset, y0 + h - 1 + offset, wave_color);
            }
        }
    }
}

static LCD_Button_Struct* btn_return_des = NULL;

static void On_Return_Click(LCD_Button_Struct* self) {
    gen_config = 0;
    self->figure.bg_color = LCD_COLOR_YELLOW;
    BSP_LCD_FillRect(self->figure.x, self->figure.y, self->figure.w, self->figure.h, self->figure.bg_color);
    BSP_LCD_DrawString(self->figure.x + FONT_OFFSET_X, self->figure.y + FONT_OFFSET_Y, self->txt, self->font_color, self->font_type, self->figure.bg_color);
    BSP_DWT_Delay_ms(200);
    des_config = 1;
}

static LCD_Button_Struct* btn_control_ch1 = NULL;
static LCD_Button_Struct* btn_control_ch2 = NULL;

static void On_Channel_Toggle(LCD_Button_Struct* self) {
    if (strcmp(self->figure.inner_name, "btn_ch1") == 0) {
        self->figure.bg_color = (self->figure.bg_color == LCD_COLOR_DARKGREEN) ? 0xFF333333 : LCD_COLOR_DARKGREEN;
        self->clicked = (self->clicked == 0) ? 1 : 0;
        BSP_LCD_FillRect(self->figure.x, self->figure.y, self->figure.w, self->figure.h, self->figure.bg_color);
        BSP_LCD_DrawString(self->figure.x + FONT_OFFSET_X, self->figure.y + FONT_OFFSET_Y, self->txt, self->font_color, self->font_type, self->figure.bg_color);
        Control_DAC_Enable(1, (self->clicked));
        // TODO:按钮控制(ok)
    } else if (strcmp(self->figure.inner_name, "btn_ch2") == 0){
        self->figure.bg_color = (self->figure.bg_color == LCD_COLOR_RED) ? 0xFF333333 : LCD_COLOR_RED;
        self->clicked = (self->clicked == 0) ? 1 : 0;
        BSP_LCD_FillRect(self->figure.x, self->figure.y, self->figure.w, self->figure.h, self->figure.bg_color);
        BSP_LCD_DrawString(self->figure.x + FONT_OFFSET_X, self->figure.y + FONT_OFFSET_Y, self->txt, self->font_color, self->font_type, self->figure.bg_color);
        Control_DAC_Enable(2, (self->clicked));
        // 同理
    }
}

static LCD_TXT_Struct* txt_ch1_vpp_freq  = NULL;
static LCD_TXT_Struct* txt_ch2_vpp_freq  = NULL;

static void Refresh_TXT(LCD_TXT_Struct* self, const char* txt) {
    strcpy(self->txt, txt);
    BSP_LCD_DrawString(self->figure.x + FONT_OFFSET_X, self->figure.y + FONT_OFFSET_Y, txt, self->font_color, self->font_type, self->figure.bg_color);
}

static LCD_Button_Struct* btn_wftype_ch1 = NULL;
static LCD_Button_Struct* btn_wftype_ch2 = NULL;

static void Button_Waveform_Type(LCD_Button_Struct* self) {
    if (strcmp(self->figure.inner_name, "wf_ch1") == 0) {
        self->figure.bg_color = (self->figure.bg_color == LCD_COLOR_DARKGREEN) ? 0xFF333333 : LCD_COLOR_DARKGREEN;
        BSP_LCD_FillRect(self->figure.x, self->figure.y, self->figure.w, self->figure.h, self->figure.bg_color);
        BSP_DWT_Delay_ms(200);
        self->figure.bg_color = (self->figure.bg_color == LCD_COLOR_DARKGREEN) ? 0xFF333333 : LCD_COLOR_DARKGREEN;
        BSP_LCD_FillRect(self->figure.x, self->figure.y, self->figure.w, self->figure.h, self->figure.bg_color);
        
        uint8_t wf_type = SINE_TYPE;
        if (self->clicked == 0) {
            self->clicked = 1;
            wf_type = SQUARE_TYPE;
            strcpy(self->txt, "SQU");
        } else if (self->clicked == 1) {
            self->clicked = 2;
            wf_type = TRIANGLE_TYPE;
            strcpy(self->txt, "TRI");
        } else if (self->clicked == 2) {
            self->clicked = 0;
            wf_type = SINE_TYPE;
            strcpy(self->txt, "SIN");
        }
        BSP_LCD_DrawString(self->figure.x + FONT_OFFSET_X, self->figure.y + FONT_OFFSET_Y, self->txt, self->font_color, self->font_type, self->figure.bg_color);
        Switch_Picture_Waveform_Draw(&gen_pic_ch1, wf_type);
        Set_DAC_Wave_Type(1, wf_type);
    } else if (strcmp(self->figure.inner_name, "wf_ch2") == 0) {
        self->figure.bg_color = (self->figure.bg_color == LCD_COLOR_RED) ? 0xFF333333 : LCD_COLOR_RED;
        BSP_LCD_FillRect(self->figure.x, self->figure.y, self->figure.w, self->figure.h, self->figure.bg_color);
        BSP_DWT_Delay_ms(200);
        self->figure.bg_color = (self->figure.bg_color == LCD_COLOR_RED) ? 0xFF333333 : LCD_COLOR_RED;
        BSP_LCD_FillRect(self->figure.x, self->figure.y, self->figure.w, self->figure.h, self->figure.bg_color);
        
        uint8_t wf_type = SINE_TYPE;
        if (self->clicked == 0) {
            self->clicked = 1;
            wf_type = SQUARE_TYPE;
            strcpy(self->txt, "SQU");
        } else if (self->clicked == 1) {
            self->clicked = 2;
            wf_type = TRIANGLE_TYPE;
            strcpy(self->txt, "TRI");
        } else if (self->clicked == 2) {
            self->clicked = 0;
            wf_type = SINE_TYPE;
            strcpy(self->txt, "SIN");
        }
        BSP_LCD_DrawString(self->figure.x + FONT_OFFSET_X, self->figure.y + FONT_OFFSET_Y, self->txt, self->font_color, self->font_type, self->figure.bg_color);
        Switch_Picture_Waveform_Draw(&gen_pic_ch2, wf_type);
        Set_DAC_Wave_Type(2, wf_type);
    }
}

static LCD_Button_Struct* btn_vpp_plus_ch1 = NULL;
static LCD_Button_Struct* btn_vpp_minus_ch1 = NULL;
static LCD_Button_Struct* btn_vpp_plus_ch2 = NULL;
static LCD_Button_Struct* btn_vpp_minus_ch2 = NULL;

static void Button_Vpp_Adjust(LCD_Button_Struct* self) {
    if ((strcmp(self->figure.inner_name, "vpp_plus_ch1") == 0) || (strcmp(self->figure.inner_name, "vpp_minus_ch1") == 0)) {
        self->figure.bg_color = (self->figure.bg_color == LCD_COLOR_DARKGREEN) ? 0xFF333333 : LCD_COLOR_DARKGREEN;
        BSP_LCD_FillRect(self->figure.x, self->figure.y, self->figure.w, self->figure.h, self->figure.bg_color);
        BSP_DWT_Delay_ms(50);
        self->figure.bg_color = (self->figure.bg_color == LCD_COLOR_DARKGREEN) ? 0xFF333333 : LCD_COLOR_DARKGREEN;
        BSP_LCD_FillRect(self->figure.x, self->figure.y, self->figure.w, self->figure.h, self->figure.bg_color);

        float current_vpp = Get_DAC_Vpp(1);
        if (strcmp(self->figure.inner_name, "vpp_plus_ch1") == 0) {
            current_vpp += VPP_STEP;
            if (current_vpp > MAX_VPP) {
                current_vpp = MAX_VPP;
            }
        } else {
            current_vpp -= VPP_STEP;
            if (current_vpp < MIN_VPP) {
                current_vpp = MIN_VPP;
            }
        }   
        BSP_LCD_DrawString(self->figure.x + FONT_OFFSET_X, self->figure.y + FONT_OFFSET_Y, self->txt, self->font_color, self->font_type, self->figure.bg_color);

        char txt[MAX_TXT_LENGTH] = {0};
        sprintf(txt, "CH1: Vpp=%.1fV Freq=%uHz Duty=%u%%%s", current_vpp, Get_DAC_Freq(1), Get_DAC_Duty(1), filled_txt);
        Set_DAC_Vpp(1, current_vpp);

        txt_ch1_vpp_freq->refresh_txt(txt_ch1_vpp_freq, txt);
    } else if ((strcmp(self->figure.inner_name, "vpp_plus_ch2") == 0) || (strcmp(self->figure.inner_name, "vpp_minus_ch2") == 0)) {
        self->figure.bg_color = (self->figure.bg_color == LCD_COLOR_RED) ? 0xFF333333 : LCD_COLOR_RED;
        BSP_LCD_FillRect(self->figure.x, self->figure.y, self->figure.w, self->figure.h, self->figure.bg_color);
        BSP_DWT_Delay_ms(50);
        self->figure.bg_color = (self->figure.bg_color == LCD_COLOR_RED) ? 0xFF333333 : LCD_COLOR_RED;
        BSP_LCD_FillRect(self->figure.x, self->figure.y, self->figure.w, self->figure.h, self->figure.bg_color);

        float current_vpp = Get_DAC_Vpp(2);
        if (strcmp(self->figure.inner_name, "vpp_plus_ch2") == 0) {
            current_vpp += VPP_STEP;
            if (current_vpp > MAX_VPP) {
                current_vpp = MAX_VPP;
            }
        } else {
            current_vpp -= VPP_STEP;
            if (current_vpp < MIN_VPP) {
                current_vpp = MIN_VPP;
            }
        }   
        BSP_LCD_DrawString(self->figure.x + FONT_OFFSET_X, self->figure.y + FONT_OFFSET_Y, self->txt, self->font_color, self->font_type, self->figure.bg_color);

        char txt[MAX_TXT_LENGTH] = {0};
        sprintf(txt, "CH2: Vpp=%.1fV Freq=%uHz Duty=%u%%%s", current_vpp, Get_DAC_Freq(2), Get_DAC_Duty(2), filled_txt);
        Set_DAC_Vpp(2, current_vpp);
        txt_ch2_vpp_freq->refresh_txt(txt_ch2_vpp_freq, txt);
    }
}

static LCD_Button_Struct* btn_freq_plus_ch1 = NULL;
static LCD_Button_Struct* btn_freq_minus_ch1 = NULL;
static LCD_Button_Struct* btn_freq_plus_ch2 = NULL;
static LCD_Button_Struct* btn_freq_minus_ch2 = NULL;

static void Button_Freq_Adjust(LCD_Button_Struct* self) {
    if ((strcmp(self->figure.inner_name, "freq_plus_ch1") == 0) || (strcmp(self->figure.inner_name, "freq_minus_ch1") == 0)) {
        self->figure.bg_color = (self->figure.bg_color == LCD_COLOR_DARKGREEN) ? 0xFF333333 : LCD_COLOR_DARKGREEN;
        BSP_LCD_FillRect(self->figure.x, self->figure.y, self->figure.w, self->figure.h, self->figure.bg_color);
        BSP_DWT_Delay_ms(50);
        self->figure.bg_color = (self->figure.bg_color == LCD_COLOR_DARKGREEN) ? 0xFF333333 : LCD_COLOR_DARKGREEN;
        BSP_LCD_FillRect(self->figure.x, self->figure.y, self->figure.w, self->figure.h, self->figure.bg_color);

        uint32_t current_freq = Get_DAC_Freq(1);
        if (strcmp(self->figure.inner_name, "freq_plus_ch1") == 0) {
            current_freq += FREQ_STEP;
            if (current_freq > MAX_FREQ) {
                current_freq = MAX_FREQ;
            }
        } else {
            current_freq -= FREQ_STEP;
            if (current_freq < MIN_FREQ) {
                current_freq = MIN_FREQ;
            }
        }   
        BSP_LCD_DrawString(self->figure.x + FONT_OFFSET_X, self->figure.y + FONT_OFFSET_Y, self->txt, self->font_color, self->font_type, self->figure.bg_color);

        char txt[MAX_TXT_LENGTH] = {0};
        sprintf(txt, "CH1: Vpp=%.1fV Freq=%uHz Duty=%u%%%s", Get_DAC_Vpp(1), current_freq, Get_DAC_Duty(1), filled_txt);
        Set_DAC_Freq(1, current_freq);
        txt_ch1_vpp_freq->refresh_txt(txt_ch1_vpp_freq, txt);
    } else if ((strcmp(self->figure.inner_name, "freq_plus_ch2") == 0) || (strcmp(self->figure.inner_name, "freq_minus_ch2") == 0)) {
        self->figure.bg_color = (self->figure.bg_color == LCD_COLOR_RED) ? 0xFF333333 : LCD_COLOR_RED;
        BSP_LCD_FillRect(self->figure.x, self->figure.y, self->figure.w, self->figure.h, self->figure.bg_color);
        BSP_DWT_Delay_ms(50);
        self->figure.bg_color = (self->figure.bg_color == LCD_COLOR_RED) ? 0xFF333333 : LCD_COLOR_RED;
        BSP_LCD_FillRect(self->figure.x, self->figure.y, self->figure.w, self->figure.h, self->figure.bg_color);

        uint32_t current_freq = Get_DAC_Freq(2);
        if (strcmp(self->figure.inner_name, "freq_plus_ch2") == 0) {
            current_freq += FREQ_STEP;
            if (current_freq > MAX_FREQ) {
                current_freq = MAX_FREQ;
            }
        } else {
            current_freq -= FREQ_STEP;
            if (current_freq < MIN_FREQ) {
                current_freq = MIN_FREQ;
            }
        }
        BSP_LCD_DrawString(self->figure.x + FONT_OFFSET_X, self->figure.y + FONT_OFFSET_Y, self->txt, self->font_color, self->font_type, self->figure.bg_color);

        char txt[MAX_TXT_LENGTH] = {0};
        sprintf(txt, "CH2: Vpp=%.1fV Freq=%uHz Duty=%u%%%s", Get_DAC_Vpp(2), current_freq, Get_DAC_Duty(2), filled_txt);
        Set_DAC_Freq(2, current_freq);
        txt_ch2_vpp_freq->refresh_txt(txt_ch2_vpp_freq, txt);
    }
}

static LCD_Button_Struct* btn_duty_plus_ch1 = NULL;
static LCD_Button_Struct* btn_duty_minus_ch1 = NULL;
static LCD_Button_Struct* btn_duty_plus_ch2 = NULL;
static LCD_Button_Struct* btn_duty_minus_ch2 = NULL;

static void Button_Duty_Adjust(LCD_Button_Struct* self) {
    if ((strcmp(self->figure.inner_name, "duty_plus_ch1") == 0) || (strcmp(self->figure.inner_name, "duty_minus_ch1") == 0)) {
        self->figure.bg_color = (self->figure.bg_color == LCD_COLOR_DARKGREEN) ? 0xFF333333 : LCD_COLOR_DARKGREEN;
        BSP_LCD_FillRect(self->figure.x, self->figure.y, self->figure.w, self->figure.h, self->figure.bg_color);
        BSP_DWT_Delay_ms(50);
        self->figure.bg_color = (self->figure.bg_color == LCD_COLOR_DARKGREEN) ? 0xFF333333 : LCD_COLOR_DARKGREEN;
        BSP_LCD_FillRect(self->figure.x, self->figure.y, self->figure.w, self->figure.h, self->figure.bg_color);

        uint32_t current_duty = Get_DAC_Duty(1);
        if (strcmp(self->figure.inner_name, "duty_plus_ch1") == 0) {
            current_duty += DUTY_STEP;
            if (current_duty > MAX_DUTY) {
                current_duty = MAX_DUTY;
            }
        } else {
            current_duty -= DUTY_STEP;
            if (current_duty < MIN_DUTY) {
                current_duty = MIN_DUTY;
            }
        }
        BSP_LCD_DrawString(self->figure.x + FONT_OFFSET_X, self->figure.y + FONT_OFFSET_Y, self->txt, self->font_color, self->font_type, self->figure.bg_color);

        char txt[MAX_TXT_LENGTH] = {0};
        sprintf(txt, "CH1: Vpp=%.1fV Freq=%uHz Duty=%u%%%s", Get_DAC_Vpp(1), Get_DAC_Freq(1), current_duty, filled_txt);
        Set_DAC_Duty(1, current_duty);
        txt_ch1_vpp_freq->refresh_txt(txt_ch1_vpp_freq, txt);
    } else if ((strcmp(self->figure.inner_name, "duty_plus_ch2") == 0) || (strcmp(self->figure.inner_name, "duty_minus_ch2") == 0)) {
        self->figure.bg_color = (self->figure.bg_color == LCD_COLOR_RED) ? 0xFF333333 : LCD_COLOR_RED;
        BSP_LCD_FillRect(self->figure.x, self->figure.y, self->figure.w, self->figure.h, self->figure.bg_color);
        BSP_DWT_Delay_ms(50);
        self->figure.bg_color = (self->figure.bg_color == LCD_COLOR_RED) ? 0xFF333333 : LCD_COLOR_RED;
        BSP_LCD_FillRect(self->figure.x, self->figure.y, self->figure.w, self->figure.h, self->figure.bg_color);

        uint32_t current_duty = Get_DAC_Duty(2);
        if (strcmp(self->figure.inner_name, "duty_plus_ch2") == 0) {
            current_duty += DUTY_STEP;
            if (current_duty > MAX_DUTY) {
                current_duty = MAX_DUTY;
            }
        } else {
            current_duty -= DUTY_STEP;
            if (current_duty < MIN_DUTY) {
                current_duty = MIN_DUTY;
            }
        }
        BSP_LCD_DrawString(self->figure.x + FONT_OFFSET_X, self->figure.y + FONT_OFFSET_Y, self->txt, self->font_color, self->font_type, self->figure.bg_color);

        char txt[MAX_TXT_LENGTH] = {0};
        sprintf(txt, "CH2: Vpp=%.1fV Freq=%uHz Duty=%u%%%s", Get_DAC_Vpp(2), Get_DAC_Freq(2), current_duty, filled_txt);
        Set_DAC_Duty(2, current_duty);
        txt_ch2_vpp_freq->refresh_txt(txt_ch2_vpp_freq, txt);
    }
}

void GEN_Core_Init(void) {
    DAC_Output_Init();
}

void GEN_Page_Init(void) {
    gen_config = 1;

    LCD_UI_ClearPool();
    BSP_LCD_Clear(LCD_COLOR_BLACK);

    btn_return_des = LCD_UI_CreateButton("btn_return", 10, 10, 80, 40, LCD_COLOR_DARKGREEN, "ReturnDES", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
    btn_return_des->on_click = On_Return_Click;

	char temp[64] = {0};
	sprintf(temp, "CH1: Vpp=3.0V Freq=1000Hz Duty=50%%%s", filled_txt);
    txt_ch1_vpp_freq = LCD_UI_CreateTXT("ch1_vpp_freq", 20, 170, 260, 32, LCD_COLOR_BLUE, temp, ASCII_FONT_TYPE_8x16, LCD_COLOR_WHITE);
    txt_ch1_vpp_freq->refresh_txt = Refresh_TXT;
	sprintf(temp, "CH2: Vpp=3.0V Freq=1000Hz Duty=50%%%s", filled_txt);
    txt_ch2_vpp_freq = LCD_UI_CreateTXT("ch2_vpp_freq", 460, 170, 260, 32, LCD_COLOR_BLUE, temp, ASCII_FONT_TYPE_8x16, LCD_COLOR_WHITE);
    txt_ch2_vpp_freq->refresh_txt = Refresh_TXT;

    btn_control_ch1 = LCD_UI_CreateButton("btn_ch1", 20, 210, 80, 50, LCD_COLOR_DARKGREEN, "CH1", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
    btn_control_ch1->on_click = On_Channel_Toggle;
    btn_wftype_ch1 = LCD_UI_CreateButton("wf_ch1", 110, 210, 80, 50, LCD_COLOR_DARKGREEN, "SIN", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
    btn_wftype_ch1->on_click = Button_Waveform_Type;

    btn_vpp_plus_ch1 = LCD_UI_CreateButton("vpp_plus_ch1", 20, 270, 80, 50, LCD_COLOR_DARKGREEN, "100mV+", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
    btn_vpp_plus_ch1->on_click = Button_Vpp_Adjust;
    btn_vpp_minus_ch1 = LCD_UI_CreateButton("vpp_minus_ch1", 110, 270, 80, 50, LCD_COLOR_DARKGREEN, "100mV-", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
    btn_vpp_minus_ch1->on_click = Button_Vpp_Adjust;

    btn_freq_plus_ch1 = LCD_UI_CreateButton("freq_plus_ch1", 20, 330, 80, 50, LCD_COLOR_DARKGREEN, "1kHz+", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
    btn_freq_plus_ch1->on_click = Button_Freq_Adjust;
    btn_freq_minus_ch1 = LCD_UI_CreateButton("freq_minus_ch1", 110, 330, 80, 50, LCD_COLOR_DARKGREEN, "1kHz-", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
    btn_freq_minus_ch1->on_click = Button_Freq_Adjust;

    btn_duty_plus_ch1 = LCD_UI_CreateButton("duty_plus_ch1", 20, 390, 80, 50, LCD_COLOR_DARKGREEN, "1%+", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
    btn_duty_plus_ch1->on_click = Button_Duty_Adjust;
    btn_duty_minus_ch1 = LCD_UI_CreateButton("duty_minus_ch1", 110, 390, 80, 50, LCD_COLOR_DARKGREEN, "1%-", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
    btn_duty_minus_ch1->on_click = Button_Duty_Adjust;

    btn_control_ch2 = LCD_UI_CreateButton("btn_ch2", 460, 210, 80, 50, LCD_COLOR_RED, "CH2", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
    btn_control_ch2->on_click = On_Channel_Toggle;
    btn_wftype_ch2 = LCD_UI_CreateButton("wf_ch2", 550, 210, 80, 50, LCD_COLOR_RED, "SIN", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
    btn_wftype_ch2->on_click = Button_Waveform_Type;

    btn_vpp_plus_ch2 = LCD_UI_CreateButton("vpp_plus_ch2", 460, 270, 80, 50, LCD_COLOR_RED, "100mV+", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
    btn_vpp_plus_ch2->on_click = Button_Vpp_Adjust;
    btn_vpp_minus_ch2 = LCD_UI_CreateButton("vpp_minus_ch2", 550, 270, 80, 50, LCD_COLOR_RED, "100mV-", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
    btn_vpp_minus_ch2->on_click = Button_Vpp_Adjust;

    btn_freq_plus_ch2 = LCD_UI_CreateButton("freq_plus_ch2", 460, 330, 80, 50, LCD_COLOR_RED, "1kHz+", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
    btn_freq_plus_ch2->on_click = Button_Freq_Adjust;
    btn_freq_minus_ch2 = LCD_UI_CreateButton("freq_minus_ch2", 550, 330, 80, 50, LCD_COLOR_RED, "1kHz-", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
    btn_freq_minus_ch2->on_click = Button_Freq_Adjust;

    btn_duty_plus_ch2 = LCD_UI_CreateButton("duty_plus_ch2", 460, 390, 80, 50, LCD_COLOR_RED, "1%+", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
    btn_duty_plus_ch2->on_click = Button_Duty_Adjust;
    btn_duty_minus_ch2 = LCD_UI_CreateButton("duty_minus_ch2", 550, 390, 80, 50, LCD_COLOR_RED, "1%-", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
    btn_duty_minus_ch2->on_click = Button_Duty_Adjust;

    Create_GPS(&gen_pic_ch1, 20, 60, 240, 100, LCD_COLOR_WHITE, LCD_COLOR_DARKGREEN, SINE_TYPE, 3);
    Create_GPS(&gen_pic_ch2, 460, 60, 240, 100, LCD_COLOR_WHITE, LCD_COLOR_RED, SINE_TYPE, 3);
    Switch_Picture_Waveform_Draw(&gen_pic_ch1, gen_pic_ch1.wf_type);
    Switch_Picture_Waveform_Draw(&gen_pic_ch2, gen_pic_ch2.wf_type);

    LCD_UI_Render_All();
}

void GEN_Logic_Running(uint8_t changed) {
    if (changed) {
        Calc_DAC_Buffer(1);
        Calc_DAC_Buffer(2);
    }
}
