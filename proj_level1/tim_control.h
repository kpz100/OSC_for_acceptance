/**
 * @file tim_control.h
 * @brief Timer and clock control interface
 * 
 * Provides API for managing extern clock generation (SI5351) and timer sync.
 */

#ifndef TIM_CONTROL_H
#define TIM_CONTROL_H

#include "main.h"

/** Initialize timer and SI5351 oscillator system */
void Tim_Control_Init(void);

/** Get current frequency setting for a channel (Hz) */
uint32_t Get_Tim_Freq(uint8_t ch);

/** Set output frequency for a channel (Hz) */
void Set_Tim_Freq(uint8_t ch, uint32_t freq);

/** Enable/disable timer clock output for a channel */
void Control_Tim_Clk(uint8_t ch, uint8_t enable);

#endif