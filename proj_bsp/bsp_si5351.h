#ifndef __BSP_SI5351_H
#define __BSP_SI5351_H

#include "main.h"

#define SI5351_ADDR (0x60 << 1)

typedef enum {
    SI5351_DRIVE_2MA = 0,
    SI5351_DRIVE_4MA = 1,
    SI5351_DRIVE_6MA = 2,
    SI5351_DRIVE_8MA = 3
} SI5351_Drive_MA; // Output drive strength in milliamps

/* Function declarations */
// Initialize SI5351, set load capacitance to 8pF and disable all outputs
void BSP_SI5351_Init(void);

// Enable/disable outputs based on mask (bit 0=CLK0, bit 1=CLK1, bit 2=CLK2)
void BSP_SI5351_EnableOutputs(uint8_t enabled_mask);

// Enable or disable individual clock output
void BSP_SI5351_EnableControl(uint8_t clk_id, uint8_t enable);

// Configure CLK0 output frequency and drive strength
void BSP_SI5351_SetupCLK0(int32_t freq_hz, SI5351_Drive_MA drive_mA);

// Configure CLK2 output frequency and drive strength
void BSP_SI5351_SetupCLK2(int32_t freq_hz, SI5351_Drive_MA drive_mA);

#endif /* __BSP_SI5351_H */