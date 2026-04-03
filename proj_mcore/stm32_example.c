// STM32 Usage Example
// Waveform classification using only: waveform array, sampling_mode, frequency, amplitude
// #include "waveform_classifier.c"
// #include "feature_extractor.c"

// ADC DMA buffer (1024 samples, 8-bit format)
extern uint8_t adc_buffer[1024];

void classify_waveform(void) {
    // Known parameters (can be measured or configured)
    uint8_t sampling_mode = 0;        // 0: 36x oversampling, 1: 100x oversampling
    float frequency_hz = 50000.0f;    // Measured frequency in Hz
    uint16_t amplitude_mv = 1650;     // Measured Vpp in millivolts (1.65V)
    
    float features[21];
    extract_features(adc_buffer, 1024, sampling_mode, frequency_hz, amplitude_mv, features);
    uint8_t waveform_type = predict_waveform(features);
    
    // waveform_type: 0 = Sine, 1 = Square, 2 = Triangle, 3 = Other
    switch(waveform_type) {
        case 0: // Sine wave
            break;
        case 1: // Square wave
            break;
        case 2: // Triangle wave
            break;
        case 3: // Other/Unknown
            break;
    }
}
