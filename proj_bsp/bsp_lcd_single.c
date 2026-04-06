#include "bsp_lcd_single.h"
#include "ltdc.h"
#include "dma2d.h"

#ifndef abs
#define abs(x) ((x) > 0) ? (x) : -(x)
#endif

uint32_t lcd_frame_buffer[LCD_WIDTH * LCD_HEIGHT] __attribute__((used, section(".bss.LCD_FRAME_BUFFER"))) __ALIGNED(32);

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

    // 每次填充前确保模式正确，防止被 DrawRGBBlock 修改后的状态干扰
    hdma2d.Instance = DMA2D;
    hdma2d.Init.Mode = DMA2D_R2M;
    hdma2d.Init.ColorMode = DMA2D_OUTPUT_ARGB8888;
    hdma2d.Init.OutputOffset = LCD_WIDTH - w; 

    if (HAL_DMA2D_Init(&hdma2d) == HAL_OK) {
        // 启动前清理目标区域的 Cache，防止 CPU 读取到旧的 Cache 数据
        SCB_CleanInvalidateDCache_by_Addr((uint32_t *)dest_addr, h * LCD_WIDTH * 4);
        
        if (HAL_DMA2D_Start(&hdma2d, color, dest_addr, w, h) == HAL_OK) {
            HAL_DMA2D_PollForTransfer(&hdma2d, 50);
        }
    }
}

void BSP_LCD_DrawRGBBlock(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t *pBuffer)
{
    uint32_t dest_addr = (uint32_t)SDRAM_START_ADDR + ((y * LCD_WIDTH + x) * 4);

    hdma2d.Instance = DMA2D;
    hdma2d.Init.Mode = DMA2D_M2M; 
    hdma2d.Init.ColorMode = DMA2D_OUTPUT_ARGB8888;
    hdma2d.Init.OutputOffset = LCD_WIDTH - w; 
    
    if (HAL_DMA2D_Init(&hdma2d) == HAL_OK)
    {
        // 1. 将 CPU 修改过的 pBuffer 同步到内存，供 DMA2D 读取
        SCB_CleanDCache_by_Addr(pBuffer, w * h * 4);
        
        // 2. 使目标地址的 Cache 失效，确保传输完成后 CPU 必须从内存读取新数据
        SCB_InvalidateDCache_by_Addr((uint32_t *)dest_addr, h * LCD_WIDTH * 4);
        
        if (HAL_DMA2D_Start(&hdma2d, (uint32_t)pBuffer, dest_addr, w, h) == HAL_OK) {
            HAL_DMA2D_PollForTransfer(&hdma2d, 100);
        }
        
        // 注意：不需要在这里“手动切回”，FillRect 启动时会自己重新 Init。
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

void BSP_LCD_Draw_Line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color) 
{
    if (x1 >= LCD_WIDTH || y1 >= LCD_HEIGHT || x2 >= LCD_WIDTH || y2 >= LCD_HEIGHT) return;

    int16_t dx = abs(x2 - x1);
    int16_t dy = abs(y2 - y1);
    int16_t sx = (x1 < x2) ? 1 : -1; 
    int16_t sy = (y1 < y2) ? 1 : -1; 

    if (y1 == y2) 
    {
        uint16_t start_x = (x1 < x2) ? x1 : x2;
        uint16_t end_x   = (x1 < x2) ? x2 : x1;
        
        uint32_t *p_mem = (uint32_t *)SDRAM_START_ADDR + (y1 * LCD_WIDTH + start_x);
        uint16_t width = end_x - start_x + 1;
        
        while (width--) 
        {
            *p_mem++ = color;
        }
        return;
    }

    if (x1 == x2) 
    {
        uint16_t start_y = (y1 < y2) ? y1 : y2;
        uint16_t end_y   = (y1 < y2) ? y2 : y1;
        
        uint32_t *p_mem = (uint32_t *)SDRAM_START_ADDR + (start_y * LCD_WIDTH + x1);
        uint16_t height = end_y - start_y + 1;
        
        while (height--) 
        {
            *p_mem = color;
            p_mem += LCD_WIDTH;
        }
        return;
    }

    int16_t err = dx - dy; 
    int16_t e2;

    while (1) 
    {
        BSP_LCD_DrawPixel(x1, y1, color);

        if (x1 == x2 && y1 == y2) break;

        e2 = 2 * err;

        if (e2 > -dy) 
        {
            err -= dy;
            x1 += sx;
        }

        if (e2 < dx) 
        {
            err += dx;
            y1 += sy;
        }
    }
}

void BSP_LCD_Draw_Frame_Fast(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t thickness, uint32_t color) {
    if (thickness == 0) return;

    if (thickness * 2 >= w || thickness * 2 >= h) {
        BSP_LCD_FillRect(x, y, w, h, color);
        return;
    }

    BSP_LCD_FillRect(x, y, w, thickness, color);
    BSP_LCD_FillRect(x, y + h - thickness, w, thickness, color);
    BSP_LCD_FillRect(x, y + thickness, thickness, h - 2 * thickness, color);
    BSP_LCD_FillRect(x + w - thickness, y + thickness, thickness, h - 2 * thickness, color);
}

#include "ascii_font.h"

void BSP_LCD_DrawString(uint16_t x, uint16_t y, const char* str, uint32_t color, uint8_t font_type, uint32_t bg_color)
{
    while (*str) {
        char c = *str++;
        if (c < 32 || c > 126) continue; // 仅支持ASCII可打印字符
        
        const uint8_t* char_data = Get_Ascii_Font(c, font_type);

        if (font_type == ASCII_FONT_TYPE_8x16) {
            for (uint8_t row = 0; row < 16; row++) {
                uint8_t row_data = char_data[row];
                for (uint8_t col = 0; col < 8; col++) {
                    if (row_data & (1 << (7 - col))) {
                        BSP_LCD_DrawPixel(x + col, y + row, color);
                    } else {
                        BSP_LCD_DrawPixel(x + col, y + row, bg_color); // 绘制背景颜色
                    }
                }
            }
            x += 8; // 字符宽度为8像素
        } else if (font_type == ASCII_FONT_TYPE_16x32) {
            for (uint8_t row = 0; row < 32; row++) {
                // 每行占 2 个字节
                uint16_t row_data = (char_data[row * 2] << 8) | char_data[row * 2 + 1];
                for (uint8_t col = 0; col < 16; col++) {
                    if (row_data & (1 << (15 - col))) {
                        BSP_LCD_DrawPixel(x + col, y + row, color);
                    } else {
                        BSP_LCD_DrawPixel(x + col, y + row, bg_color); // 绘制背景颜色
                    }
                }
            }
            x += 16;
        } else {
            // 不支持的字体类型
            break;
        }
    }
}
