/**
 * @file gen_logic.c
 * @brief Signal generator control page logic
 * 
 * Dual-channel waveform generator UI with real-time parameter control:
 * - Waveform type selection (sine, square, triangle)
 * - Frequency adjustment (1 kHz to 50 kHz, 1 kHz steps)
 * - Amplitude control (0.1 V to 3.3 V, 0.1 V steps)
 * - Duty cycle adjustment (1-99%, 1% steps for square/triangle waves)
 * - Waveform preview pane showing current signal shape
 * - Dual-channel independent control with color-coded UI (green/red)
 */

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

/* =========================== Configuration Constants =========================== */

// Voltage adjustment parameters
#define MAX_VPP 3.3f
#define MIN_VPP 0.1f
#define VPP_STEP 0.1f

// Frequency adjustment parameters
#define MAX_FREQ 50000u
#define MIN_FREQ 1000u
#define FREQ_STEP 100u

// Duty cycle adjustment parameters
#define MAX_DUTY 99u
#define MIN_DUTY 1u
#define DUTY_STEP 1u

// Waveform type definitions
#define SINE_TYPE 0u
#define SQUARE_TYPE 1u
#define TRIANGLE_TYPE 2u

/* =========================== Global State =========================== */

uint8_t gen_config = 0;     // Signal generator page active flag

/* =========================== Data Structures =========================== */

/**
 * Waveform preview picture structure
 * Stores visual representation parameters for preview panes
 */
typedef struct Gen_Picture_Struct {
	LCD_Figure_Struct figure;    // Base figure with position and dimensions
	uint32_t wf_color;           // Waveform line color (channel-specific)
	uint8_t wf_type;             // Current waveform type (SINE/SQUARE/TRIANGLE)
	uint8_t line_width;          // Preview line thickness
} Gen_Picture_Struct;

/* =========================== Static Data =========================== */

static Gen_Picture_Struct gen_pic_ch1;   // Channel 1 waveform preview
static Gen_Picture_Struct gen_pic_ch2;   // Channel 2 waveform preview

/* =========================== Utility Functions =========================== */

/**
 * Initialize waveform preview structure with position and appearance
 * @param gps: Pointer to generator picture structure
 * @param x: X coordinate of preview pane
 * @param y: Y coordinate of preview pane
 * @param w: Width of preview pane
 * @param h: Height of preview pane
 * @param bg_color: Background color of preview
 * @param wf_color: Waveform line color (green for CH1, red for CH2)
 * @param wf_type: Initial waveform type
 * @param line_width: Thickness of drawn waveform
 */
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

/**
 * Calculate sine wave Y coordinate for given X position
 * Used for rendering smooth sine waveform in preview pane
 * @param x: Current X position in preview (0 to width-1)
 * @param width: Total preview width in pixels
 * @param height: Total preview height in pixels
 * @return: Y position scaled to preview coordinates
 */
static uint16_t Get_Sinewave_Y(uint16_t x, uint16_t width, uint16_t height) {
	float ratio = (float)x / (float)(width - 1);
	float y_ratio = (sinf(ratio * 2 * 3.14159f) + 1) / 2;
	uint16_t y_pos = (uint16_t)((1.0f - y_ratio) * (height - 1));
	return y_pos;
}

/**
 * Draw vertical line with configurable width
 * Used for square wave edges in preview
 * @param x: X coordinate of vertical line
 * @param y_start: Starting Y coordinate
 * @param y_end: Ending Y coordinate
 * @param color: Line color
 * @param line_width: Thickness of line from center
 */
static void Draw_Vertical_Line(uint16_t x, uint16_t y_start, uint16_t y_end, uint32_t color, uint8_t line_width) {
	int16_t half = line_width / 2;
	for (int16_t dx = -half; dx <= half; dx++) {
		BSP_LCD_Draw_Line(x + dx, y_start, x + dx, y_end, color);
	}
}

/**
 * Draw horizontal line with configurable width
 * Used for square wave flat portions in preview
 * @param x_start: Starting X coordinate
 * @param x_end: Ending X coordinate
 * @param y: Y coordinate of horizontal line
 * @param color: Line color
 * @param line_width: Thickness of line from center
 */
static void Draw_Horizontal_Line(uint16_t x_start, uint16_t x_end, uint16_t y, uint32_t color, uint8_t line_width) {
	int16_t half = line_width / 2;
	for (int16_t dy = -half; dy <= half; dy++) {
		BSP_LCD_Draw_Line(x_start, y + dy, x_end, y + dy, color);
	}
}

/**
 * Draw waveform preview based on selected type
 * Renders sine (smooth curve), square (rectangular), or triangle waveform
 * @param self: Pointer to generator picture structure containing position and dimensions
 * @param wf_type: Waveform type to render (SINE_TYPE, SQUARE_TYPE, or TRIANGLE_TYPE)
 * 
 * Algorithm:
 * - SINE_TYPE: Uses sinf() with phase wraparound to create smooth curve
 * - SQUARE_TYPE: Horizontal line for duty cycle, vertical edges at transitions
 * - TRIANGLE_TYPE: Diagonal lines for rising and falling slopes
 */
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

		// Clear preview pane to background color
		BSP_LCD_FillRect(x0, y0, w, h, bg_color);

		if (type == SINE_TYPE) {
			// Draw smooth sine wave using vertical sample points
			for (uint16_t x = 0; x < w; x++) {
				uint16_t wave_y = Get_Sinewave_Y(x, w, h);
				Draw_Vertical_Line(x0 + x, y0 + wave_y - 1, y0 + wave_y + 1, wave_color, line_width);
			}
		} 
		else if (type == SQUARE_TYPE) {
			// Draw rectangular square wave (50% duty shown)
			uint16_t mid_x = w / 2;
			Draw_Horizontal_Line(x0, x0 + mid_x, y0, wave_color, line_width);
			Draw_Vertical_Line(x0 + mid_x, y0, y0 + h - 1, wave_color, line_width);
			Draw_Horizontal_Line(x0 + mid_x, x0 + w - 1, y0 + h - 1, wave_color, line_width);
		} 
		else if (type == TRIANGLE_TYPE) {
			// Draw triangle wave with symmetrical slopes
			uint16_t mid_x = w / 2;
			int16_t half = line_width / 2;
			for (int16_t offset = -half; offset <= half; offset++) {
				BSP_LCD_Draw_Line(x0 + offset, y0 + h - 1 + offset, x0 + mid_x + offset, y0 + offset, wave_color);
				BSP_LCD_Draw_Line(x0 + mid_x + offset, y0 + offset, x0 + w - 1 + offset, y0 + h - 1 + offset, wave_color);
			}
		}
	}
}

/* =========================== Button Handler Helper Functions =========================== */

/**
 * Unified button visual feedback: toggle color and delay
 * @param btn: Pointer to button structure
 * @param active_color: Color when button is active (green for CH1, red for CH2)
 * @param inactive_color: Dark color (0xFF333333) when button is inactive
 */
static void Toggle_Button_Color(LCD_Button_Struct* btn, uint32_t active_color, uint32_t inactive_color) {
	if (!btn) return;
	btn->figure.bg_color = (btn->figure.bg_color == active_color) ? inactive_color : active_color;
	BSP_LCD_FillRect(btn->figure.x, btn->figure.y, btn->figure.w, btn->figure.h, btn->figure.bg_color);
	BSP_DWT_Delay_ms(200);
	btn->figure.bg_color = (btn->figure.bg_color == active_color) ? inactive_color : active_color;
	BSP_LCD_FillRect(btn->figure.x, btn->figure.y, btn->figure.w, btn->figure.h, btn->figure.bg_color);
	BSP_LCD_DrawString(btn->figure.x + FONT_OFFSET_X, btn->figure.y + FONT_OFFSET_Y, btn->txt, btn->font_color, btn->font_type, btn->figure.bg_color);
}

/**
 * Unified parameter adjustment button feedback (shorter delay for frequency/duty)
 * @param btn: Pointer to button structure
 * @param active_color: Color when button is active
 * @param inactive_color: Dark color when button is inactive
 */
static void Quick_Toggle_Button_Color(LCD_Button_Struct* btn, uint32_t active_color, uint32_t inactive_color) {
	if (!btn) return;
	btn->figure.bg_color = (btn->figure.bg_color == active_color) ? inactive_color : active_color;
	BSP_LCD_FillRect(btn->figure.x, btn->figure.y, btn->figure.w, btn->figure.h, btn->figure.bg_color);
	BSP_DWT_Delay_ms(50);
	btn->figure.bg_color = (btn->figure.bg_color == active_color) ? inactive_color : active_color;
	BSP_LCD_FillRect(btn->figure.x, btn->figure.y, btn->figure.w, btn->figure.h, btn->figure.bg_color);
	BSP_LCD_DrawString(btn->figure.x + FONT_OFFSET_X, btn->figure.y + FONT_OFFSET_Y, btn->txt, btn->font_color, btn->font_type, btn->figure.bg_color);
}

/**
 * Refresh parameter display text for channel
 * @param txt_widget: Pointer to text widget
 * @param new_text: New text to display
 */
static void Refresh_TXT(LCD_TXT_Struct* txt_widget, const char* new_text) {
	if (!txt_widget) return;
	strcpy(txt_widget->txt, new_text);
	BSP_LCD_DrawString(txt_widget->figure.x + FONT_OFFSET_X, txt_widget->figure.y + FONT_OFFSET_Y, new_text, txt_widget->font_color, txt_widget->font_type, txt_widget->figure.bg_color);
}

/* =========================== Static Button and Display Objects =========================== */

static LCD_Button_Struct* btn_return_des = NULL;
static LCD_Button_Struct* btn_control_ch1 = NULL;
static LCD_Button_Struct* btn_control_ch2 = NULL;
static LCD_Button_Struct* btn_wftype_ch1 = NULL;
static LCD_Button_Struct* btn_wftype_ch2 = NULL;
static LCD_Button_Struct* btn_vpp_plus_ch1 = NULL;
static LCD_Button_Struct* btn_vpp_minus_ch1 = NULL;
static LCD_Button_Struct* btn_vpp_plus_ch2 = NULL;
static LCD_Button_Struct* btn_vpp_minus_ch2 = NULL;
static LCD_Button_Struct* btn_freq_plus_ch1 = NULL;
static LCD_Button_Struct* btn_freq_minus_ch1 = NULL;
static LCD_Button_Struct* btn_freq_plus_ch2 = NULL;
static LCD_Button_Struct* btn_freq_minus_ch2 = NULL;
static LCD_Button_Struct* btn_duty_plus_ch1 = NULL;
static LCD_Button_Struct* btn_duty_minus_ch1 = NULL;
static LCD_Button_Struct* btn_duty_plus_ch2 = NULL;
static LCD_Button_Struct* btn_duty_minus_ch2 = NULL;
static LCD_TXT_Struct* txt_ch1_vpp_freq  = NULL;
static LCD_TXT_Struct* txt_ch2_vpp_freq  = NULL;

/* =========================== Button Event Handlers =========================== */

/**
 * Handle return to desktop page
 * Switches generator page off and desktop page on
 */
static void On_Return_Click(LCD_Button_Struct* self) {
	gen_config = 0;
	self->figure.bg_color = LCD_COLOR_YELLOW;
	BSP_LCD_FillRect(self->figure.x, self->figure.y, self->figure.w, self->figure.h, self->figure.bg_color);
	BSP_LCD_DrawString(self->figure.x + FONT_OFFSET_X, self->figure.y + FONT_OFFSET_Y, self->txt, self->font_color, self->font_type, self->figure.bg_color);
	BSP_DWT_Delay_ms(200);
	des_config = 1;
}

/**
 * Handle channel enable/disable toggle
 * Also toggles DAC output for corresponding channel
 * @param self: Button structure with inner_name identifying channel (btn_ch1 or btn_ch2)
 */
static void On_Channel_Toggle(LCD_Button_Struct* self) {
	if (!self) return;
	
	if (strcmp(self->figure.inner_name, "btn_ch1") == 0) {
		// CH1 toggle: green active, dark inactive
		Toggle_Button_Color(self, LCD_COLOR_DARKGREEN, 0xFF333333);
		self->clicked = (self->clicked == 0) ? 1 : 0;
		Control_DAC_Enable(1, self->clicked);
	} 
	else if (strcmp(self->figure.inner_name, "btn_ch2") == 0) {
		// CH2 toggle: red active, dark inactive
		Toggle_Button_Color(self, LCD_COLOR_RED, 0xFF333333);
		self->clicked = (self->clicked == 0) ? 1 : 0;
		Control_DAC_Enable(2, self->clicked);
	}
}

/**
 * Handle waveform type selection: SIN → SQU → TRI → SIN (cycling)
 * Updates both preview display and DAC output
 * @param self: Button structure with inner_name identifying channel (wf_ch1 or wf_ch2)
 */
static void Button_Waveform_Type(LCD_Button_Struct* self) {
	if (!self) return;
	
	if (strcmp(self->figure.inner_name, "wf_ch1") == 0) {
		Toggle_Button_Color(self, LCD_COLOR_DARKGREEN, 0xFF333333);
		
		uint8_t wf_type = SINE_TYPE;
		if (self->clicked == 0) {
			self->clicked = 1;
			wf_type = SQUARE_TYPE;
			strcpy(self->txt, "SQU");
		} 
		else if (self->clicked == 1) {
			self->clicked = 2;
			wf_type = TRIANGLE_TYPE;
			strcpy(self->txt, "TRI");
		} 
		else if (self->clicked == 2) {
			self->clicked = 0;
			wf_type = SINE_TYPE;
			strcpy(self->txt, "SIN");
		}
		BSP_LCD_DrawString(self->figure.x + FONT_OFFSET_X, self->figure.y + FONT_OFFSET_Y, self->txt, self->font_color, self->font_type, self->figure.bg_color);
		Switch_Picture_Waveform_Draw(&gen_pic_ch1, wf_type);
		Set_DAC_Wave_Type(1, wf_type);
	} 
	else if (strcmp(self->figure.inner_name, "wf_ch2") == 0) {
		Toggle_Button_Color(self, LCD_COLOR_RED, 0xFF333333);
		
		uint8_t wf_type = SINE_TYPE;
		if (self->clicked == 0) {
			self->clicked = 1;
			wf_type = SQUARE_TYPE;
			strcpy(self->txt, "SQU");
		} 
		else if (self->clicked == 1) {
			self->clicked = 2;
			wf_type = TRIANGLE_TYPE;
			strcpy(self->txt, "TRI");
		} 
		else if (self->clicked == 2) {
			self->clicked = 0;
			wf_type = SINE_TYPE;
			strcpy(self->txt, "SIN");
		}
		BSP_LCD_DrawString(self->figure.x + FONT_OFFSET_X, self->figure.y + FONT_OFFSET_Y, self->txt, self->font_color, self->font_type, self->figure.bg_color);
		Switch_Picture_Waveform_Draw(&gen_pic_ch2, wf_type);
		Set_DAC_Wave_Type(2, wf_type);
	}
}

/**
 * Handle voltage amplitude adjustment (Vpp)
 * Increments or decrements by VPP_STEP with clamping to [MIN_VPP, MAX_VPP]
 * @param self: Button structure with inner_name identifying channel and direction
 *              (vpp_plus_ch1, vpp_minus_ch1, vpp_plus_ch2, vpp_minus_ch2)
 */
static void Button_Vpp_Adjust(LCD_Button_Struct* self) {
	if (!self) return;
	
	if ((strcmp(self->figure.inner_name, "vpp_plus_ch1") == 0) || (strcmp(self->figure.inner_name, "vpp_minus_ch1") == 0)) {
		Quick_Toggle_Button_Color(self, LCD_COLOR_DARKGREEN, 0xFF333333);

		float current_vpp = Get_DAC_Vpp(1);
		if (strcmp(self->figure.inner_name, "vpp_plus_ch1") == 0) {
			current_vpp += VPP_STEP;
			if (current_vpp > MAX_VPP) current_vpp = MAX_VPP;
		} 
		else {
			current_vpp -= VPP_STEP;
			if (current_vpp < MIN_VPP) current_vpp = MIN_VPP;
		}
		Set_DAC_Vpp(1, current_vpp);

		char txt[MAX_TXT_LENGTH] = {0};
		sprintf(txt, "CH1: Vpp=%.1fV Freq=%uHz Duty=%u%%%s", current_vpp, Get_DAC_Freq(1), Get_DAC_Duty(1), filled_txt);
		txt_ch1_vpp_freq->refresh_txt(txt_ch1_vpp_freq, txt);
	} 
	else if ((strcmp(self->figure.inner_name, "vpp_plus_ch2") == 0) || (strcmp(self->figure.inner_name, "vpp_minus_ch2") == 0)) {
		Quick_Toggle_Button_Color(self, LCD_COLOR_RED, 0xFF333333);

		float current_vpp = Get_DAC_Vpp(2);
		if (strcmp(self->figure.inner_name, "vpp_plus_ch2") == 0) {
			current_vpp += VPP_STEP;
			if (current_vpp > MAX_VPP) current_vpp = MAX_VPP;
		} 
		else {
			current_vpp -= VPP_STEP;
			if (current_vpp < MIN_VPP) current_vpp = MIN_VPP;
		}
		Set_DAC_Vpp(2, current_vpp);

		char txt[MAX_TXT_LENGTH] = {0};
		sprintf(txt, "CH2: Vpp=%.1fV Freq=%uHz Duty=%u%%%s", current_vpp, Get_DAC_Freq(2), Get_DAC_Duty(2), filled_txt);
		txt_ch2_vpp_freq->refresh_txt(txt_ch2_vpp_freq, txt);
	}
}

/**
 * Handle frequency adjustment
 * Increments or decrements by FREQ_STEP (1 kHz) with clamping to [MIN_FREQ, MAX_FREQ]
 * @param self: Button structure with inner_name identifying channel and direction
 *              (freq_plus_ch1, freq_minus_ch1, freq_plus_ch2, freq_minus_ch2)
 */
static void Button_Freq_Adjust(LCD_Button_Struct* self) {
	if (!self) return;
	
	if ((strcmp(self->figure.inner_name, "freq_plus_ch1") == 0) || (strcmp(self->figure.inner_name, "freq_minus_ch1") == 0)) {
		Quick_Toggle_Button_Color(self, LCD_COLOR_DARKGREEN, 0xFF333333);

		uint32_t current_freq = Get_DAC_Freq(1);
		if (strcmp(self->figure.inner_name, "freq_plus_ch1") == 0) {
			current_freq += FREQ_STEP;
			if (current_freq > MAX_FREQ) current_freq = MAX_FREQ;
		} 
		else {
			current_freq -= FREQ_STEP;
			if (current_freq < MIN_FREQ) current_freq = MIN_FREQ;
		}
		Set_DAC_Freq(1, current_freq);

		char txt[MAX_TXT_LENGTH] = {0};
		sprintf(txt, "CH1: Vpp=%.1fV Freq=%uHz Duty=%u%%%s", Get_DAC_Vpp(1), current_freq, Get_DAC_Duty(1), filled_txt);
		txt_ch1_vpp_freq->refresh_txt(txt_ch1_vpp_freq, txt);
	} 
	else if ((strcmp(self->figure.inner_name, "freq_plus_ch2") == 0) || (strcmp(self->figure.inner_name, "freq_minus_ch2") == 0)) {
		Quick_Toggle_Button_Color(self, LCD_COLOR_RED, 0xFF333333);

		uint32_t current_freq = Get_DAC_Freq(2);
		if (strcmp(self->figure.inner_name, "freq_plus_ch2") == 0) {
			current_freq += FREQ_STEP;
			if (current_freq > MAX_FREQ) current_freq = MAX_FREQ;
		} 
		else {
			current_freq -= FREQ_STEP;
			if (current_freq < MIN_FREQ) current_freq = MIN_FREQ;
		}
		Set_DAC_Freq(2, current_freq);

		char txt[MAX_TXT_LENGTH] = {0};
		sprintf(txt, "CH2: Vpp=%.1fV Freq=%uHz Duty=%u%%%s", Get_DAC_Vpp(2), current_freq, Get_DAC_Duty(2), filled_txt);
		txt_ch2_vpp_freq->refresh_txt(txt_ch2_vpp_freq, txt);
	}
}

/**
 * Handle duty cycle adjustment (affects square and triangle waves)
 * Increments or decrements by DUTY_STEP (1%) with clamping to [MIN_DUTY, MAX_DUTY]
 * @param self: Button structure with inner_name identifying channel and direction
 *              (duty_plus_ch1, duty_minus_ch1, duty_plus_ch2, duty_minus_ch2)
 */
static void Button_Duty_Adjust(LCD_Button_Struct* self) {
	if (!self) return;
	
	if ((strcmp(self->figure.inner_name, "duty_plus_ch1") == 0) || (strcmp(self->figure.inner_name, "duty_minus_ch1") == 0)) {
		Quick_Toggle_Button_Color(self, LCD_COLOR_DARKGREEN, 0xFF333333);

		uint32_t current_duty = Get_DAC_Duty(1);
		if (strcmp(self->figure.inner_name, "duty_plus_ch1") == 0) {
			current_duty += DUTY_STEP;
			if (current_duty > MAX_DUTY) current_duty = MAX_DUTY;
		} 
		else {
			current_duty -= DUTY_STEP;
			if (current_duty < MIN_DUTY) current_duty = MIN_DUTY;
		}
		Set_DAC_Duty(1, current_duty);

		char txt[MAX_TXT_LENGTH] = {0};
		sprintf(txt, "CH1: Vpp=%.1fV Freq=%uHz Duty=%u%%%s", Get_DAC_Vpp(1), Get_DAC_Freq(1), current_duty, filled_txt);
		txt_ch1_vpp_freq->refresh_txt(txt_ch1_vpp_freq, txt);
	} 
	else if ((strcmp(self->figure.inner_name, "duty_plus_ch2") == 0) || (strcmp(self->figure.inner_name, "duty_minus_ch2") == 0)) {
		Quick_Toggle_Button_Color(self, LCD_COLOR_RED, 0xFF333333);

		uint32_t current_duty = Get_DAC_Duty(2);
		if (strcmp(self->figure.inner_name, "duty_plus_ch2") == 0) {
			current_duty += DUTY_STEP;
			if (current_duty > MAX_DUTY) current_duty = MAX_DUTY;
		} 
		else {
			current_duty -= DUTY_STEP;
			if (current_duty < MIN_DUTY) current_duty = MIN_DUTY;
		}
		Set_DAC_Duty(2, current_duty);

		char txt[MAX_TXT_LENGTH] = {0};
		sprintf(txt, "CH2: Vpp=%.1fV Freq=%uHz Duty=%u%%%s", Get_DAC_Vpp(2), Get_DAC_Freq(2), current_duty, filled_txt);
		txt_ch2_vpp_freq->refresh_txt(txt_ch2_vpp_freq, txt);
	}
}

/* =========================== Page Initialization =========================== */

/**
 * Initialize generator core: DAC output subsystem
 * Called once at system startup to prepare DAC hardware
 */
void GEN_Core_Init(void) {
	DAC_Output_Init();
}

/**
 * Initialize generator page UI and layout
 * Creates all buttons, text displays, and waveform preview panes
 * Sets up dual-channel control interface with synchronized layout
 * Layout:
 * - Top: Return button, parameter display text
 * - Middle: Channel toggle and waveform type buttons
 * - Bottom: Parameter adjustment buttons (Vpp, Freq, Duty)
 * - Side panels: Waveform preview panes for visual feedback
 */
void GEN_Page_Init(void) {
	gen_config = 1;

	LCD_UI_ClearPool();
	BSP_LCD_Clear(LCD_COLOR_BLACK);

	// Return to desktop button
	btn_return_des = LCD_UI_CreateButton("btn_return", 10, 10, 80, 40, LCD_COLOR_DARKGREEN, "ReturnDES", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
	btn_return_des->on_click = On_Return_Click;

	// Parameter display text areas
	char temp[64] = {0};
	sprintf(temp, "CH1: Vpp=3.0V Freq=1000Hz Duty=50%%%s", filled_txt);
	txt_ch1_vpp_freq = LCD_UI_CreateTXT("ch1_vpp_freq", 20, 170, 260, 32, LCD_COLOR_BLUE, temp, ASCII_FONT_TYPE_8x16, LCD_COLOR_WHITE);
	txt_ch1_vpp_freq->refresh_txt = Refresh_TXT;
	sprintf(temp, "CH2: Vpp=3.0V Freq=1000Hz Duty=50%%%s", filled_txt);
	txt_ch2_vpp_freq = LCD_UI_CreateTXT("ch2_vpp_freq", 460, 170, 260, 32, LCD_COLOR_BLUE, temp, ASCII_FONT_TYPE_8x16, LCD_COLOR_WHITE);
	txt_ch2_vpp_freq->refresh_txt = Refresh_TXT;

	// Channel 1 control buttons (green color scheme)
	btn_control_ch1 = LCD_UI_CreateButton("btn_ch1", 20, 210, 80, 50, LCD_COLOR_DARKGREEN, "CH1", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
	btn_control_ch1->on_click = On_Channel_Toggle;
	btn_wftype_ch1 = LCD_UI_CreateButton("wf_ch1", 130, 210, 80, 50, LCD_COLOR_DARKGREEN, "SIN", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
	btn_wftype_ch1->on_click = Button_Waveform_Type;

	btn_vpp_plus_ch1 = LCD_UI_CreateButton("vpp_plus_ch1", 20, 270, 80, 50, LCD_COLOR_DARKGREEN, "100mV+", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
	btn_vpp_plus_ch1->on_click = Button_Vpp_Adjust;
	btn_vpp_minus_ch1 = LCD_UI_CreateButton("vpp_minus_ch1", 130, 270, 80, 50, LCD_COLOR_DARKGREEN, "100mV-", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
	btn_vpp_minus_ch1->on_click = Button_Vpp_Adjust;

	btn_freq_plus_ch1 = LCD_UI_CreateButton("freq_plus_ch1", 20, 330, 80, 50, LCD_COLOR_DARKGREEN, "100Hz+", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
	btn_freq_plus_ch1->on_click = Button_Freq_Adjust;
	btn_freq_minus_ch1 = LCD_UI_CreateButton("freq_minus_ch1", 130, 330, 80, 50, LCD_COLOR_DARKGREEN, "100Hz-", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
	btn_freq_minus_ch1->on_click = Button_Freq_Adjust;

	btn_duty_plus_ch1 = LCD_UI_CreateButton("duty_plus_ch1", 20, 390, 80, 50, LCD_COLOR_DARKGREEN, "1%+", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
	btn_duty_plus_ch1->on_click = Button_Duty_Adjust;
	btn_duty_minus_ch1 = LCD_UI_CreateButton("duty_minus_ch1", 130, 390, 80, 50, LCD_COLOR_DARKGREEN, "1%-", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
	btn_duty_minus_ch1->on_click = Button_Duty_Adjust;

	// Channel 2 control buttons (red color scheme)
	btn_control_ch2 = LCD_UI_CreateButton("btn_ch2", 460, 210, 80, 50, LCD_COLOR_RED, "CH2", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
	btn_control_ch2->on_click = On_Channel_Toggle;
	btn_wftype_ch2 = LCD_UI_CreateButton("wf_ch2", 570, 210, 80, 50, LCD_COLOR_RED, "SIN", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
	btn_wftype_ch2->on_click = Button_Waveform_Type;

	btn_vpp_plus_ch2 = LCD_UI_CreateButton("vpp_plus_ch2", 460, 270, 80, 50, LCD_COLOR_RED, "100mV+", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
	btn_vpp_plus_ch2->on_click = Button_Vpp_Adjust;
	btn_vpp_minus_ch2 = LCD_UI_CreateButton("vpp_minus_ch2", 570, 270, 80, 50, LCD_COLOR_RED, "100mV-", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
	btn_vpp_minus_ch2->on_click = Button_Vpp_Adjust;

	btn_freq_plus_ch2 = LCD_UI_CreateButton("freq_plus_ch2", 460, 330, 80, 50, LCD_COLOR_RED, "100Hz+", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
	btn_freq_plus_ch2->on_click = Button_Freq_Adjust;
	btn_freq_minus_ch2 = LCD_UI_CreateButton("freq_minus_ch2", 570, 330, 80, 50, LCD_COLOR_RED, "100Hz-", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
	btn_freq_minus_ch2->on_click = Button_Freq_Adjust;

	btn_duty_plus_ch2 = LCD_UI_CreateButton("duty_plus_ch2", 460, 390, 80, 50, LCD_COLOR_RED, "1%+", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
	btn_duty_plus_ch2->on_click = Button_Duty_Adjust;
	btn_duty_minus_ch2 = LCD_UI_CreateButton("duty_minus_ch2", 570, 390, 80, 50, LCD_COLOR_RED, "1%-", ASCII_FONT_TYPE_16x32, LCD_COLOR_WHITE);
	btn_duty_minus_ch2->on_click = Button_Duty_Adjust;

	// Waveform preview panes (left side for CH1, right side for CH2)
	Create_GPS(&gen_pic_ch1, 20, 60, 240, 100, LCD_COLOR_WHITE, LCD_COLOR_DARKGREEN, SINE_TYPE, 3);
	Create_GPS(&gen_pic_ch2, 460, 60, 240, 100, LCD_COLOR_WHITE, LCD_COLOR_RED, SINE_TYPE, 3);
	Switch_Picture_Waveform_Draw(&gen_pic_ch1, gen_pic_ch1.wf_type);
	Switch_Picture_Waveform_Draw(&gen_pic_ch2, gen_pic_ch2.wf_type);

	LCD_UI_Render_All();
}

/* =========================== Main Loop Processing =========================== */

/**
 * Generator page main loop logic
 * Updates waveform output buffers when parameters change
 * @param changed: Flag indicating if any parameter was modified (1 = recalculate, 0 = no change)
 */
void GEN_Logic_Running(uint8_t changed) {
	if (changed) {
		Calc_DAC_Buffer(1);
		Calc_DAC_Buffer(2);
	}
}
