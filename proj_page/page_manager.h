#ifndef PAGE_MANAGER_H
#define PAGE_MANAGER_H

#include "main.h"

extern uint8_t des_config;
extern uint8_t osc_config;
extern uint8_t gen_config;

uint8_t Page_Touch_Logic(void);
void DES_Page_Init(void);

void OSC_Core_Init(void);
void OSC_Page_Init(void);
void OSC_Page_Refresh_Data(float vpp1, float fft1, float vpp2, float fft2);

void GEN_Core_Init(void);
void GEN_Page_Init(void);
void GEN_Logic_Running(uint8_t changed);

#endif