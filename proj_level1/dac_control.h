/**
 * @file dac_control.h
 * @brief DAC waveform generation interface
 * 
 * Supports three waveform types: sine, square, triangle
 * with configurable frequency, voltage, and duty cycle.
 */

#ifndef DAC_CONTROL_H
#define DAC_CONTROL_H

#include "main.h"

/* =========================== Configuration Constants =========================== */

#define DAC_LENGTH 100u         // Samples per waveform period
#define DAC_CHANNEL_NUM 2u      // Number of DAC channels

// Mathematical and reference constants
#define PI2 6.283185307f        // 2π constant for sine wave generation
#define VERF 3.3f               // Reference voltage (3.3V)
#define DOUBLE_VERF (2.0f * VERF)  // Double reference (for sine wave offset)
#define U12BIT 4095u            // Maximum 12-bit ADC/DAC value

/* =========================== Waveform Type Enum =========================== */

#define SIN_WAVE 0              // Sine waveform
#define SQU_WAVE 1              // Square waveform
#define TRI_WAVE 2              // Triangle waveform

/* =========================== Public API =========================== */

/** Initialize DAC module and both channels */
void DAC_Output_Init(void);

/** Enable or disable DAC output for a channel */
void Control_DAC_Enable(uint8_t ch, uint8_t enable);

/** Generate waveform lookup table based on current configuration */
void Calc_DAC_Buffer(uint8_t ch);

// Configuration setters
/** Set peak-to-peak voltage (0.0-3.3V) */
void Set_DAC_Vpp(uint8_t ch, float Vpp);

/** Set waveform type (SIN_WAVE, SQU_WAVE, TRI_WAVE) */
void Set_DAC_Wave_Type(uint8_t ch, uint8_t w_type);

/** Set output frequency (Hz) */
void Set_DAC_Freq(uint8_t ch, float freq);

/** Set duty cycle for square/triangle waves (1-99%) */
void Set_DAC_Duty(uint8_t ch, uint8_t duty);

// Configuration getters
/** Get current output frequency (Hz) */
uint32_t Get_DAC_Freq(uint8_t ch);

/** Get peak-to-peak voltage (V) */
float Get_DAC_Vpp(uint8_t ch);

/** Get duty cycle (%) */
uint8_t Get_DAC_Duty(uint8_t ch);

/** Get waveform type (0=SIN, 1=SQU, 2=TRI) */
uint8_t Get_DAC_Wave_Type(uint8_t ch);

/** Get buffer update status (1=needs update, 0=current) */
uint8_t Get_DAC_Status(uint8_t ch);

#endif