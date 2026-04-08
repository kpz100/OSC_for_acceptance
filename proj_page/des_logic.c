/**
 * @file des_logic.c
 * @brief Desktop/home page logic and touch handling
 * 
 * Main landing page with navigation buttons to access:
 * - Oscilloscope page for signal acquisition and display
 * - Signal generator page for waveform output control
 */

#include "page_manager.h"
#include "ui_rampool.h"
#include "ui_type.h"
#include "ui_touch.h"

#include "bsp_touch.h"
#include "bsp_dwt.h"
#include "bsp_lcd_single.h"

#include <stdio.h>
#include <string.h>
#include "ascii_font.h"

/* =========================== Global State =========================== */

uint8_t des_config = 0;    // Desktop page active flag

/* =========================== Static Data =========================== */

static uint16_t x_touch_pos = 0;    // Current X touch position
static uint16_t y_touch_pos = 0;    // Current Y touch position

// UI elements
static LCD_TXT_Struct* txt_des_title = NULL;  // Page title text
static LCD_Button_Struct* btn_goto_osc = NULL;  // Navigate to oscilloscope
static LCD_Button_Struct* btn_goto_gen = NULL;  // Navigate to signal generator

/* =========================== Button Callbacks =========================== */

/**
 * Handle "Go to Oscilloscope" button click
 * Animates button press and switches to oscilloscope page
 */
static void On_Goto_OSC_Click(LCD_Button_Struct* self) {
	des_config = 0;
	
	// Visual feedback: toggle button color
	uint32_t color = self->figure.bg_color;
	self->figure.bg_color = (color == LCD_COLOR_DARKGREEN) ? LCD_COLOR_YELLOW : LCD_COLOR_DARKGREEN;
	BSP_LCD_FillRect(self->figure.x, self->figure.y, self->figure.w, self->figure.h, self->figure.bg_color);
	BSP_LCD_DrawString(self->figure.x + FONT_OFFSET_X, self->figure.y + FONT_OFFSET_Y, self->txt, self->font_color, self->font_type, self->figure.bg_color);
	
	BSP_DWT_Delay_ms(200);
	osc_config = 1;  // Switch to oscilloscope page
}

/**
 * Handle "Go to Generator" button click
 * Animates button press and switches to signal generator page
 */
static void On_Goto_GEN_Click(LCD_Button_Struct* self) {
	des_config = 0;
	
	// Visual feedback: toggle button color
	uint32_t color = self->figure.bg_color;
	self->figure.bg_color = (color == LCD_COLOR_DARKGREEN) ? LCD_COLOR_YELLOW : LCD_COLOR_DARKGREEN;
	BSP_LCD_FillRect(self->figure.x, self->figure.y, self->figure.w, self->figure.h, self->figure.bg_color);
	BSP_LCD_DrawString(self->figure.x + FONT_OFFSET_X, self->figure.y + FONT_OFFSET_Y, self->txt, self->font_color, self->font_type, self->figure.bg_color);
	
	BSP_DWT_Delay_ms(200);
	gen_config = 1;  // Switch to signal generator page
}

/* =========================== Page Initialization =========================== */

/**
 * Initialize and render the desktop page
 * Creates title and navigation buttons
 */
void DES_Page_Init(void) {
	des_config = 1;

	LCD_UI_ClearPool();
	BSP_LCD_Clear(LCD_COLOR_BLACK);

	// Create page title
	txt_des_title = LCD_UI_CreateTXT("des_title", 280, 20, 220, 40, LCD_COLOR_WHITE, "Desktop Page", ASCII_FONT_TYPE_16x32, LCD_COLOR_BLACK);

	// Create navigation buttons
	btn_goto_osc = LCD_UI_CreateButton("btn_goto_osc", 180, 200, 160, 40, LCD_COLOR_DARKGREEN, "Go to OSC", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
	btn_goto_osc->on_click = On_Goto_OSC_Click;

	btn_goto_gen = LCD_UI_CreateButton("btn_goto_gen", 480, 200, 160, 40, LCD_COLOR_DARKGREEN, "Go to GEN", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
	btn_goto_gen->on_click = On_Goto_GEN_Click;

	LCD_UI_Render_All();
}

/* =========================== Touch Input Processing =========================== */

/**
 * Process touch input events
 * Averages multi-touch coordinates and routes to UI system
 * Returns 1 if touch was detected and processed, 0 otherwise
 */
uint8_t Page_Touch_Logic(void) {
	if (GT911_Scan()) {
		// Average multi-touch coordinates
		for (uint8_t i = 0; i < touch_data.touch_num; i++) {
			x_touch_pos += touch_data.x[i];
			y_touch_pos += touch_data.y[i];
		}
		x_touch_pos /= touch_data.touch_num;
		y_touch_pos /= touch_data.touch_num;

		// Route to UI touch handler
		UI_Touch_Process(x_touch_pos, y_touch_pos);
		
		// Reset for next touch event
		x_touch_pos = 0;
		y_touch_pos = 0;

		return 1;
	}
	return 0;
}


