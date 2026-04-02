#ifndef OSC_MANAGER_H
#define OSC_MANAGER_H

#include "main.h"

uint8_t Get_OSC_Config(void);
void OSC_Page_Init(void);
void OSC_Page_Refresh_Data(float vpp1, float fft1, float vpp2, float fft2);

#endif