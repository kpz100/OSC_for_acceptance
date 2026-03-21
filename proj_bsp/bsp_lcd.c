#include "bsp_lcd.h"
#include "ltdc.h"
#include "dma2d.h"

static uint32_t * pCurrentDisplay = (uint32_t *)LCD_FB0_ADDR;
static uint32_t * pCurrentDraw    = (uint32_t *)LCD_FB1_ADDR;

uint32_t * BSP_LCD_GetDrawBuffer(void)
{
    return pCurrentDraw;
}

void BSP_LCD_Init(void) {
    hdma2d.Instance = DMA2D;
    hdma2d.Init.Mode = DMA2D_R2M;
    hdma2d.Init.ColorMode = DMA2D_OUTPUT_ARGB8888;
    hdma2d.Init.OutputOffset = LCD_WIDTH;

    if (HAL_DMA2D_Init(&hdma2d) == HAL_OK)
    {
        HAL_DMA2D_Start(&hdma2d, LCD_COLOR_BLACK, LCD_FB0_ADDR, LCD_WIDTH, LCD_HEIGHT);
        HAL_DMA2D_PollForTransfer(&hdma2d, 50);

        HAL_DMA2D_Start(&hdma2d, LCD_COLOR_BLACK, LCD_FB1_ADDR, LCD_WIDTH, LCD_HEIGHT);
        HAL_DMA2D_PollForTransfer(&hdma2d, 50);
    }
}

void BSP_LCD_Flip(void)
{
    HAL_LTDC_SetAddress(&hltdc, (uint32_t)pCurrentDraw, 0);

    __HAL_LTDC_CLEAR_FLAG(&hltdc, LTDC_FLAG_RR);

    HAL_LTDC_Reload(&hltdc, LTDC_SRCR_VBR);

    while (__HAL_LTDC_GET_FLAG(&hltdc, LTDC_FLAG_RR) == RESET);

    uint32_t * temp = pCurrentDisplay;
    pCurrentDisplay = pCurrentDraw;
    pCurrentDraw = temp;
}

void BSP_LCD_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color)
{
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    if (x + w > LCD_WIDTH)  w = LCD_WIDTH - x;
    if (y + h > LCD_HEIGHT) h = LCD_HEIGHT - y;

    uint32_t dest_addr = (uint32_t)pCurrentDraw + ((y * LCD_WIDTH + x) * 4);

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
    pCurrentDraw[y * LCD_WIDTH + x] = color;
}

