#ifndef __BSP_SDRAM_H
#define __BSP_SDRAM_H

#include "main.h"

#define SDRAM_SIZE            ((uint32_t)0x2000000)
#define SDRAM_BANK_ADDR       ((uint32_t)0xC0000000)
#define SDRAM_TIMEOUT         ((uint32_t)0x1000)

#define SDRAM_MODEREG_BURST_LENGTH_2             ((uint16_t)0x0001)
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL      ((uint16_t)0x0000)
#define SDRAM_MODEREG_CAS_LATENCY_3              ((uint16_t)0x0030)
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD    ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE     ((uint16_t)0x0200)

void     BSP_SDRAM_Init(void);
uint32_t BSP_SDRAM_SelfTest(void);

#endif