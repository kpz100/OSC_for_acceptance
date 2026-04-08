/**
 * @file osc_logic.c
 * @brief Oscilloscope acquisition, FFT analysis, and dual-channel display
 * 
 * Handles:
 * - Frequency detection using FFT analysis with automatic sampling adjustment
 * - Dual-channel waveform capture and display synchronization
 * - Real-time Vpp and frequency measurement
 * - Trigger point detection (rising edge synchronization)
 */

#include "page_manager.h"
#include "ui_rampool.h"
#include "ui_type.h"
#include "ui_touch.h"

#include "bsp_dwt.h"
#include "bsp_lcd_single.h"

#include <string.h>
#include <stdio.h>

#include "ascii_font.h"
#include "adc_control.h"
#include "tim_control.h"

/* =========================== Constants and Definitions =========================== */

#define SHOW_DELAY_MS 1000u    // Display refresh delay (ms)
#define FFT_DELAY_MS 500u      // FFT computation delay (ms)

#ifndef abs
#define abs(x) ((x) > 0) ? (x) : -(x)
#endif

/* =========================== Global State =========================== */

uint8_t osc_config = 0;     // Oscilloscope page active flag

uint32_t fft_tick = 0;      // FFT processing timer
uint32_t show_tick = 0;     // Display update timer

/* =========================== Static UI Elements =========================== */

static LCD_Waveform_Struct* wf_show_lcd = NULL;       // Main waveform display
static LCD_Button_Struct* btn_return_des = NULL;      // Return to desktop button
static LCD_Button_Struct* btn_control_ch1 = NULL;     // Channel 1 enable/disable
static LCD_Button_Struct* btn_control_ch2 = NULL;     // Channel 2 enable/disable
static LCD_TXT_Struct* txt_ch1_vpp_fft = NULL;        // Channel 1 measurement display
static LCD_TXT_Struct* txt_ch2_vpp_fft = NULL;        // Channel 2 measurement display

/* =========================== Drawing Primitives =========================== */

/**
 * Set pixel in waveform buffer
 * Bounds-checked writes to pre-allocated RGB buffer
 */
static void Buffer_SetPixel(LCD_Waveform_Struct* self, uint16_t x, uint16_t y, uint32_t color) {
	if (x >= self->figure.w || y >= self->figure.h) return;
	self->osc_draw_buffer[y * self->figure.w + x] = color;
}

/**
 * Draw line in waveform buffer using Bresenham's algorithm
 * Supports arbitrary line slopes
 */
static void Buffer_DrawLine(LCD_Waveform_Struct* self, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color) {
	int16_t dx = abs(x2 - x1);
	int16_t dy = abs(y2 - y1);
	int16_t sx = (x1 < x2) ? 1 : -1;
	int16_t sy = (y1 < y2) ? 1 : -1;
	int16_t err = dx - dy;
	int16_t e2;

	while (1) {
		Buffer_SetPixel(self, x1, y1, color);
		if (x1 == x2 && y1 == y2) break;
		e2 = 2 * err;
		if (e2 > -dy) { err -= dy; x1 += sx; }
		if (e2 < dx) { err += dx; y1 += sy; }
	}
}

/**
 * Draw waveform in buffer from various data types
 * Supports 8/16/32-bit sample formats with automatic scaling
 */
static void DrawIn_Buffer(LCD_Waveform_Struct* self, void* buffer, size_t _type, uint32_t true_maxval, uint32_t length, uint32_t color) {
	if (!self || !buffer || length == 0 || true_maxval == 0) return;

	// Calculate data compression ratio
	float x_ratio = (float)length / self->figure.w;
	if (x_ratio < 1.0f) x_ratio = 1.0f;
	
	uint16_t last_x = 0;
	uint16_t last_y = 0;
	uint8_t first_point = 1;

	// Draw waveform by sampling and interpolating
	for (uint16_t x = 0; x < self->figure.w; x++) {
		uint32_t data_idx = (uint32_t)(x * x_ratio);
		if (data_idx >= length) break;

		// Extract sample based on data type
		uint32_t raw_val = 0;
		if (_type == 1) raw_val = ((uint8_t*)buffer)[data_idx];
		else if (_type == 2) raw_val = ((uint16_t*)buffer)[data_idx];
		else if (_type == 4) raw_val = ((uint32_t*)buffer)[data_idx];

		// Clamp and scale to display coordinates
		if (raw_val > true_maxval) raw_val = true_maxval;
		uint16_t screen_y = (uint16_t)((uint64_t)(true_maxval - raw_val) * (self->figure.h - 1) / true_maxval);

		if (first_point) {
			Buffer_SetPixel(self, x, screen_y, color);
			first_point = 0;
		} else {
			Buffer_DrawLine(self, last_x, last_y, x, screen_y, color);
		}
		last_x = x; last_y = screen_y;
	}
}

/**
 * Clear waveform buffer to background color
 */
static void Bgin_Buffer(LCD_Waveform_Struct* self) {
	if (!self) return;

	uint32_t total_pixels = self->figure.w * self->figure.h;
	for (uint32_t i = 0; i < total_pixels; i++) {
		self->osc_draw_buffer[i] = self->figure.bg_color;
	} 
}

/**
 * Push buffer to LCD (DMA transfer) and synchronize
 */
static void DrawTo_LCD(LCD_Waveform_Struct* self) {
	if (!self) return;
	BSP_LCD_DrawRGBBlock(self->figure.x, self->figure.y, self->figure.w, self->figure.h, self->osc_draw_buffer);
	memset(self->osc_draw_buffer, 0, sizeof(self->osc_draw_buffer));
	BSP_DWT_Delay_ms(100);  // Prevent flicker from rapid refresh
}

/* =========================== Button Callbacks =========================== */

/**
 * Return to desktop page button handler
 */
static void On_Return_Click(LCD_Button_Struct* self) {
	osc_config = 0;
	self->figure.bg_color = LCD_COLOR_YELLOW;
	BSP_LCD_FillRect(self->figure.x, self->figure.y, self->figure.w, self->figure.h, self->figure.bg_color);
	BSP_LCD_DrawString(self->figure.x + FONT_OFFSET_X, self->figure.y + FONT_OFFSET_Y, self->txt, self->font_color, self->font_type, self->figure.bg_color);
	BSP_DWT_Delay_ms(100);
	des_config = 1;
}

/**
 * Channel enable/disable button toggle
 * Clears flags and resets target when disabling
 */
static void On_Channel_Toggle(LCD_Button_Struct* self) {
	if (strcmp(self->figure.inner_name, "btn_ch1") == 0) {
		self->figure.bg_color = (self->figure.bg_color == LCD_COLOR_DARKGREEN) ? 0xFF333333 : LCD_COLOR_DARKGREEN;
		self->clicked = (self->clicked == 0) ? 1 : 0;
		BSP_LCD_FillRect(self->figure.x, self->figure.y, self->figure.w, self->figure.h, self->figure.bg_color);
		BSP_LCD_DrawString(self->figure.x + FONT_OFFSET_X, self->figure.y + FONT_OFFSET_Y, self->txt, self->font_color, self->font_type, self->figure.bg_color);
		Control_ADC_Enable(1, self->clicked);
		if (self->clicked == 0) {
			Clear_ADC_Flag(1, FFT_FLAG_TYPE);
			Clear_ADC_Flag(1, SHOW_FLAG_TYPE);
			Set_Next_Target_Type(1, RW_TARGET_NONE);
		}
	} else if (strcmp(self->figure.inner_name, "btn_ch2") == 0){
		self->figure.bg_color = (self->figure.bg_color == LCD_COLOR_RED) ? 0xFF333333 : LCD_COLOR_RED;
		self->clicked = (self->clicked == 0) ? 1 : 0;
		BSP_LCD_FillRect(self->figure.x, self->figure.y, self->figure.w, self->figure.h, self->figure.bg_color);
		BSP_LCD_DrawString(self->figure.x + FONT_OFFSET_X, self->figure.y + FONT_OFFSET_Y, self->txt, self->font_color, self->font_type, self->figure.bg_color);
		Control_ADC_Enable(2, self->clicked);
		if (self->clicked == 0) {
			Clear_ADC_Flag(2, FFT_FLAG_TYPE);
			Clear_ADC_Flag(2, SHOW_FLAG_TYPE);
			Set_Next_Target_Type(2, RW_TARGET_NONE);
		}
	}
}

/**
 * Refresh measurement text on display
 */
static void Refresh_TXT(LCD_TXT_Struct* self, const char* txt) {
	BSP_LCD_DrawString(self->figure.x + FONT_OFFSET_X, self->figure.y + FONT_OFFSET_Y, txt, self->font_color, self->font_type, self->figure.bg_color);
}

/* =========================== FFT and Display Processing =========================== */

/**
 * Process FFT results and adjust sampling frequency for frequency locking
 * Implements hill-climbing algorithm to center target frequency
 */
static void OSC_FFT_Running(uint8_t ch) {
	if (Get_ADC_Flag(ch, FFT_FLAG_TYPE)) {
		uint8_t fft_ok = Calc_Comp_FFT_Ampl(ch);
		if (fft_ok) {
			float fft_freq = Get_FFT_Freq(ch);
			if (fft_freq < MAX_OVER_SAMPLE_FREQ) {
				Set_Sample_Freq(ch, fft_freq * OVER_SAMPLE_RATE);
			} else {
				Set_Sample_Freq(ch, fft_freq * (1.0f - 1.0f / ETS_SAMPLE_RATE));
			}

			printf("Set CH%d Sample Freq to %.2fHz\n", ch, Get_Sample_Freq(ch));

			Clear_ADC_Flag(ch, FFT_FLAG_TYPE);
			Set_Next_Target_Type(ch, RW_TARGET_SHOW);
		} else {
			Set_Sample_Freq(ch, Get_Sample_Freq(ch) - SAMPLE_FREQ_SHIFT);

			Clear_ADC_Flag(ch, FFT_FLAG_TYPE);
			Set_Next_Target_Type(ch, RW_TARGET_FFT);
		}
	} 
}

/**
 * Update measurement display with Vpp and frequency values
 */
static void OSC_Vpp_Freq_Refresh(uint8_t ch) {
	float vpp = (float)Get_Vpp_8(ch) * 3.3f / 255.0f;
	float freq = Get_FFT_Freq(ch);

	printf("Vpp: CH%d=%.2fV, Freq=%.2fHz\n", ch, vpp, freq);

	char txt[64] = {0};
	sprintf(txt, "CH%d: Vpp=%.2fV Freq=%.2fHz%s", ch, vpp, freq, filled_txt);
	if (ch == 1) {
		txt_ch1_vpp_fft->refresh_txt(txt_ch1_vpp_fft, txt);
	} else {
		txt_ch2_vpp_fft->refresh_txt(txt_ch2_vpp_fft, txt);
	}
}

/**
 * Process single-channel waveform (legacy single-channel mode)
 */
static uint8_t OSC_Perform_Running(uint8_t ch) {
	if (Get_ADC_Flag(ch, SHOW_FLAG_TYPE)) {
		Calc_Vpp8(ch);

		uint8_t* show_buffer = Get_Show_Buffer(ch);
		uint32_t RE_pos = Calc_Rising_Edge_Pos(ch);
		uint32_t available_length = Get_Available_Show_Length(RE_pos);
		
		wf_show_lcd->drawin_buffer(wf_show_lcd, &show_buffer[RE_pos], sizeof(uint8_t), 255, available_length, ((ch == 1) ? LCD_COLOR_GREEN : LCD_COLOR_RED));

		wf_show_lcd->drawto_lcd(wf_show_lcd);

		printf("Show: CH%d Available_Length=%u\n", ch, available_length);

		Clear_ADC_Flag(ch, SHOW_FLAG_TYPE);
		Set_Next_Target_Type(ch, RW_TARGET_FFT);
		Set_Sample_Freq(ch, ORIGINAL_SAMPLE_FREQ);
		
		OSC_Vpp_Freq_Refresh(ch);
		
		printf("Next Rank\n\n");

		return 1;
	}
	return 0;
}

/**
 * Dual-channel synchronized display
 * Waits for both channels to have data before pushing to screen
 */
static int8_t rank = -1;

static void OSC_Perform_Running_Else(void) {
	if (btn_control_ch1->clicked == 1 && btn_control_ch2->clicked == 1) {
		// Both channels enabled - synchronize display
		if (rank == -1) {
			wf_show_lcd->bgin_buffer(wf_show_lcd);
			rank += 1;
		}
		if (Get_ADC_Flag(1, SHOW_FLAG_TYPE)) {
			Calc_Vpp8(1);

			uint8_t* show_buffer = Get_Show_Buffer(1);
			uint32_t RE_pos = Calc_Rising_Edge_Pos(1);
			uint32_t available_length = Get_Available_Show_Length(RE_pos);
			
			wf_show_lcd->drawin_buffer(wf_show_lcd, &show_buffer[RE_pos], sizeof(uint8_t), 255, available_length, LCD_COLOR_GREEN);

			Clear_ADC_Flag(1, SHOW_FLAG_TYPE);
			Set_Next_Target_Type(1, RW_TARGET_NONE);

			rank += 1;
		}
		if (Get_ADC_Flag(2, SHOW_FLAG_TYPE)) {
			Calc_Vpp8(2);

			uint8_t* show_buffer = Get_Show_Buffer(2);
			uint32_t RE_pos = Calc_Rising_Edge_Pos(2);
			uint32_t available_length = Get_Available_Show_Length(RE_pos);
			
			wf_show_lcd->drawin_buffer(wf_show_lcd, &show_buffer[RE_pos], sizeof(uint8_t), 255, available_length, LCD_COLOR_RED);

			Clear_ADC_Flag(2, SHOW_FLAG_TYPE);
			Set_Next_Target_Type(2, RW_TARGET_NONE);

			rank += 1;
		}
		// Push to screen when both channels ready
		if (rank >= 2) {
			wf_show_lcd->drawto_lcd(wf_show_lcd);
			rank = -1;

			Set_Next_Target_Type(1, RW_TARGET_FFT);
			Set_Sample_Freq(1, ORIGINAL_SAMPLE_FREQ);
			Set_Next_Target_Type(2, RW_TARGET_FFT);
			Set_Sample_Freq(2, ORIGINAL_SAMPLE_FREQ);
			
			OSC_Vpp_Freq_Refresh(1);
			OSC_Vpp_Freq_Refresh(2);
		}
	} else if (btn_control_ch1->clicked == 1) {
		// Single channel 1 enabled
		wf_show_lcd->bgin_buffer(wf_show_lcd);
		OSC_Perform_Running(1);
		rank = -1;
	} else if (btn_control_ch2->clicked == 1) {
		// Single channel 2 enabled
		wf_show_lcd->bgin_buffer(wf_show_lcd);
		OSC_Perform_Running(2);
		rank = -1;
	}
}

/* =========================== Initialization and Main Loop =========================== */

/**
 * Initialize oscilloscope core modules (ADC, timers, FFT)
 */
void OSC_Core_Init(void) {
	fft_tick = 0;
	show_tick = 0;
	rank = -1;
	
	Tim_Control_Init();
	ADC_FFT_Init();
}

/**
 * Initialize and render oscilloscope UI page
 */
void OSC_Page_Init(void) {
	osc_config = 1;

	LCD_UI_ClearPool();
	BSP_LCD_Clear(LCD_COLOR_BLACK); 

	// Create return button
	btn_return_des = LCD_UI_CreateButton("btn_return", 10, 10, 80, 40, LCD_COLOR_DARKGREEN, "ReturnDES", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
	btn_return_des->on_click = On_Return_Click;

	// Create channel enable buttons
	btn_control_ch1 = LCD_UI_CreateButton("btn_ch1", 10, 360, 60, 50, LCD_COLOR_DARKGREEN, "CH1", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
	btn_control_ch2 = LCD_UI_CreateButton("btn_ch2", 10, 420, 60, 50, LCD_COLOR_RED, "CH2", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
	btn_control_ch1->on_click = On_Channel_Toggle;
	btn_control_ch2->on_click = On_Channel_Toggle;

	// Create measurement display
	char temp_txt[64] = {0};
	sprintf(temp_txt, "CH1: Vpp=0.00V Freq=0.00Hz%s", filled_txt);
	txt_ch1_vpp_fft = LCD_UI_CreateTXT("ch1_vpp_fft", 80, 360, 220, 32, LCD_COLOR_BLUE, temp_txt, ASCII_FONT_TYPE_8x16, LCD_COLOR_WHITE);
	sprintf(temp_txt, "CH2: Vpp=0.00V Freq=0.00Hz%s", filled_txt);
	txt_ch2_vpp_fft = LCD_UI_CreateTXT("ch2_vpp_fft", 80, 420, 220, 32, LCD_COLOR_BLUE, temp_txt, ASCII_FONT_TYPE_8x16, LCD_COLOR_WHITE);
	txt_ch1_vpp_fft->refresh_txt = Refresh_TXT;
	txt_ch2_vpp_fft->refresh_txt = Refresh_TXT;

	// Create waveform display
	wf_show_lcd = LCD_UI_CreateWaveform("wf_show", 0, 60, MAX_OSC_WIDTH, MAX_OSC_HEIGHT, LCD_COLOR_WHITE, LCD_COLOR_DARKGREEN, LCD_COLOR_GREEN, LCD_COLOR_RED);
	wf_show_lcd->drawin_buffer = DrawIn_Buffer;
	wf_show_lcd->bgin_buffer = Bgin_Buffer;
	wf_show_lcd->drawto_lcd = DrawTo_LCD;

	LCD_UI_Render_All();
}

/**
 * Main oscilloscope processing loop
 * Handles FFT computations and display updates
 */
void OSC_Logic_Running(void) {
	// Process FFT for both channels
	for (uint8_t ch = 1; ch <= 2; ch++) {
		OSC_FFT_Running(ch);
	}
	
	// Handle display updates
	OSC_Perform_Running_Else();
}
