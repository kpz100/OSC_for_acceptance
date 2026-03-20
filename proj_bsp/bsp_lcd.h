#ifndef __BSP_LCD_H
#define __BSP_LCD_H

#include "main.h"

#define LCD_WIDTH             800
#define LCD_HEIGHT            480
#define SDRAM_START_ADDR      0xC0000000
#define LCD_FRAME_SIZE        (LCD_WIDTH * LCD_HEIGHT * 4)

#define LCD_FB0_ADDR          (SDRAM_START_ADDR)
#define LCD_FB1_ADDR          (SDRAM_START_ADDR + LCD_FRAME_SIZE)

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

/**
 * @brief Initialize LCD module
 */
void BSP_LCD_Init(void);

/**
 * @brief Get the current draw buffer
 * @return Pointer to draw buffer
 */
uint32_t * BSP_LCD_GetDrawBuffer(void);

/**
 * @brief Flip display buffer to show current draw buffer
 */
void BSP_LCD_Flip(void);

/**
 * @brief Fill rectangle with color
 * @param x X coordinate
 * @param y Y coordinate
 * @param w Width
 * @param h Height
 * @param color Color value
 */
void BSP_LCD_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color);

/**
 * @brief Clear full screen with color
 * @param color Color value
 */
void BSP_LCD_Clear(uint32_t color);

/**
 * @brief Draw a single pixel
 * @param x X coordinate
 * @param y Y coordinate
 * @param color Color value
 */
void BSP_LCD_DrawPixel(uint16_t x, uint16_t y, uint32_t color);

#endif