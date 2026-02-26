#ifndef __MIXED_RADIX_2_5_F32_H__
#define __MIXED_RADIX_2_5_F32_H__

#include <stdint.h>
#include <math.h>

typedef float float_t;

// void mixed_radix_2_5_fft(float* x, int N);

void init_mixed_radix_2_5_fft_160_320(int N);
void mixed_radix_2_5_fft_160_320(float* x, int N);

#endif

