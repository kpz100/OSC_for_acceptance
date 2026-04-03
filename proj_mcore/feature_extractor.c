// Lightweight feature extraction for STM32
// Input: waveform[1024] (uint8_t), sampling_mode, freq_hz, amplitude_mv
#include <stdint.h>
#include <math.h>

void extract_features(const uint8_t* waveform, uint32_t len,
                     uint8_t sampling_mode, float freq_hz, uint16_t amplitude_mv,
                     float* out_features) {
    uint8_t min_val = 255, max_val = 0;
    uint32_t sum = 0;
    uint32_t sum_sq = 0;
    uint32_t zero_crossings = 0;
    uint8_t midpoint = 128;
    uint32_t peaks = 0, valleys = 0;
    float max_rise = 0, max_fall = 0;
    float sum_abs_slope = 0;
    
    // First pass: basic statistics
    for (uint32_t i=0; i<len; i++) {
        uint8_t v = waveform[i];
        if (v < min_val) min_val = v;
        if (v > max_val) max_val = v;
        sum += v;
        sum_sq += v*v;
        if (i>0) {
            if ((waveform[i-1]-midpoint)*(v-midpoint) < 0) zero_crossings++;
            float diff = (float)v - waveform[i-1];
            float abs_diff = fabsf(diff);
            sum_abs_slope += abs_diff;
            if (diff > max_rise) max_rise = diff;
            if (diff < max_fall) max_fall = diff;
        }
        if (i>0 && i<len-1) {
            if (v > waveform[i-1] && v > waveform[i+1]) peaks++;
            if (v < waveform[i-1] && v < waveform[i+1]) valleys++;
        }
    }
    
    float mean_val = (float)sum / len;
    float std_val = sqrtf((float)sum_sq/len - mean_val*mean_val);
    float peak_to_peak = (float)(max_val - min_val);
    float mean_abs_slope = sum_abs_slope / len;
    
    // FFT features (simplified - compute fundamental and harmonics)
    float fft_peak = 0.0f, harmonic_ratio = 0.0f, harmonic_ratio3 = 0.0f;
    float sum_cos = 0, sum_sin = 0;
    float sum_cos2 = 0, sum_sin2 = 0;
    float sum_cos3 = 0, sum_sin3 = 0;
    float samples_per_cycle = (sampling_mode == 0) ? 36.0f : 100.0f;
    float angle_step = 2.0f * 3.14159265f / samples_per_cycle;
    
    for (uint32_t i=0; i<len; i++) {
        float angle = i * angle_step;
        float val = (float)waveform[i];
        sum_cos += val * cosf(angle);
        sum_sin += val * sinf(angle);
        sum_cos2 += val * cosf(2*angle);
        sum_sin2 += val * sinf(2*angle);
        sum_cos3 += val * cosf(3*angle);
        sum_sin3 += val * sinf(3*angle);
    }
    
    fft_peak = sqrtf(sum_cos*sum_cos + sum_sin*sum_sin) / len;
    float fft_2nd = sqrtf(sum_cos2*sum_cos2 + sum_sin2*sum_sin2) / len;
    float fft_3rd = sqrtf(sum_cos3*sum_cos3 + sum_sin3*sum_sin3) / len;
    harmonic_ratio = fft_2nd / (fft_peak + 1e-6f);
    harmonic_ratio3 = fft_3rd / (fft_peak + 1e-6f);
    
    // Signal level statistics
    uint8_t high_thresh = (max_val*9 + min_val)/10;
    uint8_t low_thresh = (max_val + min_val*9)/10;
    uint32_t high_cnt = 0, low_cnt = 0;
    for (uint32_t i=0; i<len; i++) {
        if (waveform[i] > high_thresh) high_cnt++;
        if (waveform[i] < low_thresh) low_cnt++;
    }
    float above_high = (float)high_cnt / len;
    float below_low = (float)low_cnt / len;
    
    // Skewness (symmetry)
    float skewness = 0;
    for (uint32_t i=0; i<len; i++) {
        float diff = (float)waveform[i] - mean_val;
        skewness += diff * diff * diff;
    }
    skewness = skewness / len / (std_val * std_val * std_val + 1e-6f);
    
    // Find FFT peak index (simplified - find max in first 20 bins)
    float fft_peak_index = 1.0f;  // default
    float max_fft = fft_peak;
    // Note: For exact bin calculation would need more computation
    // Using the computed fundamental as proxy
    
    // Pack features (21 features total)
    out_features[0] = (float)min_val;
    out_features[1] = (float)max_val;
    out_features[2] = mean_val;
    out_features[3] = std_val;
    out_features[4] = peak_to_peak;
    out_features[5] = (float)zero_crossings;
    out_features[6] = (float)peaks;
    out_features[7] = (float)valleys;
    out_features[8] = fft_peak;
    out_features[9] = fft_peak_index;
    out_features[10] = harmonic_ratio;
    out_features[11] = harmonic_ratio3;
    out_features[12] = above_high;
    out_features[13] = below_low;
    out_features[14] = max_rise;
    out_features[15] = max_fall;
    out_features[16] = mean_abs_slope;
    out_features[17] = skewness;
    out_features[18] = freq_hz;
    out_features[19] = (float)sampling_mode;
    out_features[20] = (float)amplitude_mv;
}
