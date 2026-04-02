#ifndef __BSP_LCD_SINGLE_H
#define __BSP_LCD_SINGLE_H

#include "main.h"

#define LCD_WIDTH             800
#define LCD_HEIGHT            480

#define SDRAM_START_ADDR      0xC0000000
#define LCD_FRAME_SIZE        (LCD_WIDTH * LCD_HEIGHT * 4)

#ifndef LCD_COLOR
#define LCD_COLOR_WHITE       0xFFFFFFFF
#define LCD_COLOR_BLACK       0xFF000000
#define LCD_COLOR_RED         0xFFFF0000
#define LCD_COLOR_GREEN       0xFF00FF00
#define LCD_COLOR_BLUE        0xFF0000FF
#define LCD_COLOR_CYAN        0xFF00FFFF
#define LCD_COLOR_MAGENTA     0xFFFF00FF
#define LCD_COLOR_YELLOW      0xFFFFFF00
#define LCD_COLOR_DARKGREEN   0xFF008000
#define LCD_COLOR_TRANSPARENT 0x00000000
#endif

void BSP_LCD_Init(void);
void BSP_LCD_Clear(uint32_t color);
void BSP_LCD_DrawPixel(uint16_t x, uint16_t y, uint32_t color);
void BSP_LCD_Draw_Line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color);
void BSP_LCD_Draw_Frame_Fast(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t thickness, uint32_t color);
void BSP_LCD_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color);
void BSP_LCD_DrawRGBBlock(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t *pBuffer);
void BSP_LCD_DrawString(uint16_t x, uint16_t y, const char* str, uint32_t color, uint8_t font_type, uint32_t bg_color);

#endif