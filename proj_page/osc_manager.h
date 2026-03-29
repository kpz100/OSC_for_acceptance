#ifndef OSC_MANAGER_H
#define OSC_MANAGER_H

#include "main.h"

uint8_t Get_OSC_Config(void);
void OSC_Page_Init(void);
void OSC_Page_Refresh_Data(float v1, float f1, float v2, float f2);

#endif