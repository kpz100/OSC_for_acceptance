#include "osc_manager.h"
#include "adc_control.h"
#include "tim_control.h"
#include <string.h>

#include "bsp_lcd_single.h"
#include "ui_type.h"

extern LCD_Waveform_Struct* wf_show_lcd;

static uint32_t fft_tick = 0;

void OSC_Core_Init(void) {
    Tim_Control_Init();
    ADC_FFT_Init();

    fft_tick = 0;
}

static void OSC_FFT_Running(uint8_t ch) {
    if (Get_ADC_Flag(ch, FFT_FLAG_TYPE)) {
        uint8_t fft_ok = Calc_Comp_FFT_Ampl(ch);
        if (fft_ok) {
            float fft_freq = Get_FFT_Freq(ch);
            if (fft_freq < MAX_OVER_SAMPLE_FREQ) {
                Set_Sample_Freq(ch, fft_freq * OVER_SAMPLE_RATE);
            } else {
                Set_Sample_Freq(ch, fft_freq * (1.0f - 1.0f / ETS_SAMPLE_RATE));
            }

            Clear_ADC_Flag(ch, FFT_FLAG_TYPE, RW_TARGET_SHOW);
        } else {
            Clear_ADC_Flag(ch, FFT_FLAG_TYPE, RW_TARGET_FFT);
            Set_Sample_Freq(ch, Get_Sample_Freq(ch) - SAMPLE_FREQ_SHIFT);
        }
    } 
}

static void OSC_Perform_Running(uint8_t ch) {
    if (Get_ADC_Flag(ch, SHOW_FLAG_TYPE)) {
        Calc_Vpp8(ch);

        uint8_t* show_buffer = Get_Show_Buffer(ch);
        uint32_t RE_pos = Calc_Rising_Edge_Pos(ch);
        uint32_t available_length = Get_Available_Show_Length(RE_pos);
        
        wf_show_lcd->drawin_buffer(wf_show_lcd, &show_buffer[RE_pos], sizeof(uint8_t), 255, available_length, ((ch == 1) ? LCD_COLOR_GREEN : LCD_COLOR_RED));

        Clear_ADC_Flag(ch, SHOW_FLAG_TYPE, RW_TARGET_FFT);
    }
}

void OSC_Logic_Running(void) {
    
}
