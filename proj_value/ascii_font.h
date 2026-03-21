#ifndef ASCII_FONT_H
#define ASCII_FONT_H

#include "main.h"

#define ASCII_FONT_TYPE_8x16 0u
#define ASCII_FONT_TYPE_16x32 1u

const uint8_t * Get_Ascii_Font(uint8_t word, uint8_t font_type);

#endif