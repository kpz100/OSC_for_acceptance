#include "bsp_lcd.h"
#include "ltdc.h"
#include "dma2d.h"

void BSP_LCD_Init(void) {
    hdma2d.Instance = DMA2D;
    hdma2d.Init.Mode = DMA2D_R2M;
    hdma2d.Init.ColorMode = DMA2D_OUTPUT_ARGB8888;
    hdma2d.Init.OutputOffset = LCD_WIDTH;

    if (HAL_DMA2D_Init(&hdma2d) == HAL_OK)
    {
        HAL_DMA2D_Start(&hdma2d, LCD_COLOR_WHITE, SDRAM_START_ADDR, LCD_WIDTH, LCD_HEIGHT);
        HAL_DMA2D_PollForTransfer(&hdma2d, 50);
    }
}

void BSP_LCD_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color)
{
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    if (x + w > LCD_WIDTH)  w = LCD_WIDTH - x;
    if (y + h > LCD_HEIGHT) h = LCD_HEIGHT - y;

    uint32_t dest_addr = (uint32_t)SDRAM_START_ADDR + ((y * LCD_WIDTH + x) * 4);

    hdma2d.Instance->OOR = LCD_WIDTH - w;
    
    if (HAL_DMA2D_Start(&hdma2d, color, dest_addr, w, h) == HAL_OK) {
        HAL_DMA2D_PollForTransfer(&hdma2d, 50);
    }
}

void BSP_LCD_Clear(uint32_t color) 
{
    BSP_LCD_FillRect(0, 0, LCD_WIDTH, LCD_HEIGHT, color);
}

void BSP_LCD_DrawPixel(uint16_t x, uint16_t y, uint32_t color)
{
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    
    *(__IO uint32_t *)((uint32_t)SDRAM_START_ADDR + (y * LCD_WIDTH + x) * 4) = color;
}
