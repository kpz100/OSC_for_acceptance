#include "ui_render.h"
#include "ui_rampool.h"
#include "ui_type.h"
#include <string.h>
#include "bsp_lcd_single.h"

#define UI_TXT_OFFSET_X 5u
#define UI_TXT_OFFSET_Y 5u

// DrawString 的时候已经考虑了背景色，所以这里不需要额外填充背景了 -舍弃
// 按钮的背景也会触发
void UI_Render_All(void) {
    for (int i = 0; i < g_ui_pool.txt_count; i++) {
        LCD_TXT_Struct* p_txt = &g_ui_pool.texts[i];
        BSP_LCD_FillRect(p_txt->figure.x, p_txt->figure.y, p_txt->figure.w, p_txt->figure.h, p_txt->figure.bg_color);
        BSP_LCD_DrawString(p_txt->figure.x + UI_TXT_OFFSET_X, p_txt->figure.y + UI_TXT_OFFSET_Y, p_txt->txt, p_txt->font_color, p_txt->font_type, p_txt->figure.bg_color);
    }
    for (int i = 0; i < g_ui_pool.button_count; i++) {
        LCD_Button_Struct* btn = &g_ui_pool.buttons[i];
        BSP_LCD_FillRect(btn->figure.x, btn->figure.y, btn->figure.w, btn->figure.h, btn->figure.bg_color);
        BSP_LCD_DrawString(btn->figure.x + UI_TXT_OFFSET_X, btn->figure.y + UI_TXT_OFFSET_Y, btn->txt, btn->font_color, btn->font_type, btn->figure.bg_color);
    }
    for (int i = 0; i < g_ui_pool.wave_count; i++) {
        LCD_Waveform_Struct* wf = &g_ui_pool.waves[i];
        BSP_LCD_FillRect(wf->figure.x, wf->figure.y, wf->figure.w, wf->figure.h, wf->figure.bg_color);
        // 先拿底色当波形背景
    }
}
