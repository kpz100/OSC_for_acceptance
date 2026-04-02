#include "ui_touch.h"
#include "ui_rampool.h"
#include "ui_type.h"
#include <string.h>
#include "bsp_dwt.h"

#define DEADZONE_TIME_MS 300u // 建议设置为 300ms 左右

// 纯粹的坐标范围判定
static uint8_t Is_In_Rect(uint16_t x, uint16_t y, LCD_Figure_Struct* fig) {
    if ((x >= fig->x && x <= (fig->x + fig->w)) && 
        (y >= fig->y && y <= (fig->y + fig->h))) {
        return 1;
    }
    return 0;
}

void UI_Touch_Process(uint16_t x, uint16_t y) {
    uint32_t current_ticks = BSP_DWT_GetCounter();

    for (int i = 0; i < g_ui_pool.button_count; i++) {
        LCD_Button_Struct* btn = &g_ui_pool.buttons[i];

        // 1. 首先检查坐标是否在按钮区域内
        if (Is_In_Rect(x, y, &btn->figure)) {
            
            // 2. 检查时间死区，防止单次点击重复触发
            // 使用 BSP_DWT_GetDelta_us 计算自上次点击以来的时间
            float delta_ms = BSP_DWT_GetDelta_us(btn->click_time, current_ticks) / 1000.0f;

            if (delta_ms > DEADZONE_TIME_MS || btn->click_time == 0) {
                // 更新最后一次触发时间
                btn->click_time = current_ticks;
                
                // 执行回调
                if (btn->on_click != NULL) {
                    btn->on_click(btn);
                }
            }
        }
    }
}