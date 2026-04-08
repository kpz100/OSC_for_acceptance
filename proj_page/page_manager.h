/**
 * @file page_manager.h
 * @brief Page management and global state definitions
 * 
 * Defines page state flags, interface declarations for three UI pages:
 * - DES: Desktop/home page with navigation
 * - OSC: Oscilloscope acquisition and display page
 * - GEN: Signal generator control page
 */

#ifndef PAGE_MANAGER_H
#define PAGE_MANAGER_H

#include "main.h"

/* =========================== Page State Variables =========================== */

extern uint8_t des_config;    // Desktop page active flag
extern uint8_t osc_config;    // Oscilloscope page active flag
extern uint8_t gen_config;    // Signal generator page active flag

/* =========================== Desktop Page Interface =========================== */

/** Process touch input for current page (returns 1 if touch detected) */
uint8_t Page_Touch_Logic(void);

/** Initialize and render desktop page */
void DES_Page_Init(void);

/* =========================== Oscilloscope Page Interface =========================== */

/** Initialize oscilloscope module (ADC, timers, FFT) */
void OSC_Core_Init(void);

/** Initialize and render oscilloscope page UI */
void OSC_Page_Init(void);

/** Main oscilloscope processing loop (FFT, display updates) */
void OSC_Logic_Running(void);

/* =========================== Signal Generator Page Interface =========================== */

/** Initialize signal generator module (DAC, timers) */
void GEN_Core_Init(void);

/** Initialize and render signal generator page UI */
void GEN_Page_Init(void);

/** Signal generator update loop (parameter changes, waveform generation) */
void GEN_Logic_Running(uint8_t changed);

/* =========================== Utility Constants =========================== */

#define MAX_FILLED_LENGTH 10u
static const char filled_txt[MAX_FILLED_LENGTH] = "         "; // 10 spaces for text background clearing

#endif