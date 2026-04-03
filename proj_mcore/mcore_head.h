#ifndef MCORE_HEAD_H
#define MCORE_HEAD_H

#include <stdint.h>

void extract_features(const uint8_t* waveform, uint32_t len,
                     uint8_t sampling_mode, float freq_hz, uint16_t amplitude_mv,
                     float* out_features);
uint8_t predict_waveform(const float* features);

#endif