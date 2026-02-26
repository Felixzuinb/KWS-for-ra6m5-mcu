#include <stdio.h>
#include <math.h>
#include <string.h>
#include "mixed_radix_2_5_f32.h"
#include "debug_print.h" // 调试打印函数声明

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// 复数乘法: (a+bi) * (c+di) = (ac-bd) + (ad+bc)i
static inline void complex_multiply(float ar, float ai, float br, float bi,
                                    float *out_r, float *out_i)
{
    *out_r = ar * br - ai * bi;
    *out_i = ar * bi + ai * br;
}

// 复数加法: (a+bi) + (c+di) = (a+c) + (b+d)i
static inline void complex_add(float ar, float ai, float br, float bi,
                               float *out_r, float *out_i)
{
    *out_r = ar + br;
    *out_i = ai + bi;
}

// 复数减法
static inline void complex_sub(float ar, float ai, float br, float bi,
                               float *out_r, float *out_i)
{
    *out_r = ar - br;
    *out_i = ai - bi;
}

// 辅助函数：从交替数组中提取复数
static inline void get_complex(const float *x, int idx, float *r, float *i)
{
    *r = x[2 * idx];
    *i = x[2 * idx + 1];
}

// 辅助函数：设置复数到交替数组
static inline void set_complex(float *x, int idx, float r, float i)
{
    x[2 * idx] = r;
    x[2 * idx + 1] = i;
}

static float twiddle_radix_2_320[160][2];
static float twiddle_radix_2_160[80][2];
static float twiddle_radix_2_80[40][2];
static float twiddle_radix_2_40[20][2];
static float twiddle_radix_2_20[10][2];
static float twiddle_radix_2_10[5][2];
static float twiddle_radix_5_5[5][2];

void init_mixed_radix_2_5_fft_160_320(int N)
{
    if (N == 1)
    {
        return; // 单个元素，无需处理
    }

    if (N % 2 == 0)
    {
        // ==================== 基-2 分解 ====================
        int half = N / 2;

        // 递归
        init_mixed_radix_2_5_fft_160_320(half);

        // 合并结果（蝶形运算）
        // X(k) = E(k) + W_N^k * O(k)
        // X(k+N/2) = E(k) - W_N^k * O(k)
        for (int k = 0; k < half; k++)
        {
            float wr, wi; // 旋转因子

            // 计算旋转因子 W_N^k = exp(-2*pi*i*k/N) = cos() + i*sin()
            float angle = -2.0f * M_PI * k / N;
            wr = cosf(angle);
            wi = sinf(angle);
            switch (N)
            {
            case 320:
                twiddle_radix_2_320[k][0] = wr;
                twiddle_radix_2_320[k][1] = wi;
                break;
            case 160:
                twiddle_radix_2_160[k][0] = wr;
                twiddle_radix_2_160[k][1] = wi;
                break;
            case 80:
                twiddle_radix_2_80[k][0] = wr;
                twiddle_radix_2_80[k][1] = wi;
                break;
            case 40:
                twiddle_radix_2_40[k][0] = wr;
                twiddle_radix_2_40[k][1] = wi;
                break;
            case 20:
                twiddle_radix_2_20[k][0] = wr;
                twiddle_radix_2_20[k][1] = wi;
                break;
            case 10:
                twiddle_radix_2_10[k][0] = wr;
                twiddle_radix_2_10[k][1] = wi;
                break;
            default:
                break;
            }
        }
    }
    else if (N % 5 == 0)
    {
        // ==================== 基-5 分解 ====================
        int part = N / 5;

        // part = 1 递归结果直接返回，可以直接删除
        // for (int p = 0; p < 5; p++) {
        //     for (int i = 0; i < part; i++) {
        //     // 递归每个部分
        //     init_mixed_radix_2_5_fft_160_320(part);
        //     }
        // }

        // 预计算 W5 = exp(-2*pi*i/5) 的幂次
        float w5_r[5], w5_i[5]; // W5^0, W5^1, W5^2, W5^3, W5^4
        for (int m = 0; m < 5; m++)
        {
            float angle = -2.0f * M_PI * m / 5.0f;
            w5_r[m] = cosf(angle);
            w5_i[m] = sinf(angle);
            twiddle_radix_5_5[m][0] = w5_r[m];
            twiddle_radix_5_5[m][1] = w5_i[m];
        }
    }
    else
    {
        print("Error: N=%d 包含非2/5的质因子，只能处理 N=2^n*5^m\n", N);
        return;
    }
}

// 5080字节 ≈ 4.96KB
static float temp0[2 * 320];
static float temp1[2 * 160];
static float temp2[2 * 80];
static float temp3[2 * 40];
static float temp4[2 * 20];
static float temp5[2 * 10];
static float temp6[2 * 5];

void mixed_radix_2_5_fft_160_320(float *x, int N)
{
    if (N == 1)
    {
        return; // 单个元素，无需处理
    }

    // 临时缓冲区用于存储分解后的子FFT结果
    float *temp = NULL;
    if (N == 320)
    {
        temp = temp0;
    }
    else if (N == 160)
    {
        temp = temp1;
    }
    else if (N == 80)
    {
        temp = temp2;
    }
    else if (N == 40)
    {
        temp = temp3;
    }
    else if (N == 20)
    {
        temp = temp4;
    }
    else if (N == 10)
    {
        temp = temp5;
    }
    else if (N == 5)
    {
        temp = temp6;
    }
    else
    {
        print("Error: N=%d 包含非2/5的质因子，只能处理 N=2^n*5^m\n", N);
        return;
    }

    if (N % 2 == 0)
    {
        // ==================== 基-2 分解 ====================
        int half = N / 2;

        // 分别对偶数索引和奇数索引进行FFT
        // 偶数部分: x[0], x[2], x[4], ...
        // 奇数部分: x[1], x[3], x[5], ...

        // 提取偶数项到temp前半部分
        for (int i = 0; i < half; i++)
        {
            float r, i_val;
            get_complex(x, 2 * i, &r, &i_val);
            set_complex(temp, i, r, i_val);
        }
        // 递归偶数部分
        mixed_radix_2_5_fft_160_320(temp, half);

        // 提取奇数项到temp后半部分
        for (int i = 0; i < half; i++)
        {
            float r, i_val;
            get_complex(x, (2 * i + 1), &r, &i_val);
            set_complex(temp, half + i, r, i_val);
        }
        // 递归奇数部分
        mixed_radix_2_5_fft_160_320(temp + 2 * half, half);

        // 合并结果（蝶形运算）
        // X(k) = E(k) + W_N^k * O(k)
        // X(k+N/2) = E(k) - W_N^k * O(k)
        for (int k = 0; k < half; k++)
        {
            float er, ei, or_r, oi; // Even, Odd
            float wr, wi;           // 旋转因子
            float tor, toi;         // temp odd (W*O)
            float xr, xi;           // 结果

            // 读取 E(k) 和 O(k)
            get_complex(temp, k, &er, &ei);
            get_complex(temp, half + k, &or_r, &oi);

            // 计算旋转因子 W_N^k = exp(-2*pi*i*k/N) = cos() + i*sin()

            switch (N)
            {
            case 320:
                wr = twiddle_radix_2_320[k][0];
                wi = twiddle_radix_2_320[k][1];
                break;
            case 160:
                wr = twiddle_radix_2_160[k][0];
                wi = twiddle_radix_2_160[k][1];
                break;
            case 80:
                wr = twiddle_radix_2_80[k][0];
                wi = twiddle_radix_2_80[k][1];
                break;
            case 40:
                wr = twiddle_radix_2_40[k][0];
                wi = twiddle_radix_2_40[k][1];
                break;
            case 20:
                wr = twiddle_radix_2_20[k][0];
                wi = twiddle_radix_2_20[k][1];
                break;
            case 10:
                wr = twiddle_radix_2_10[k][0];
                wi = twiddle_radix_2_10[k][1];
                break;
            default:
                break;
            }

            // W_N^k * O(k)
            complex_multiply(wr, wi, or_r, oi, &tor, &toi);

            // X(k) = E(k) + W*O(k)
            complex_add(er, ei, tor, toi, &xr, &xi);
            set_complex(x, k, xr, xi);

            // X(k+N/2) = E(k) - W*O(k)
            complex_sub(er, ei, tor, toi, &xr, &xi);
            set_complex(x, (k + half), xr, xi);
        }
    }
    else if (N % 5 == 0)
    {
        // ==================== 基-5 分解 ====================
        int part = N / 5;

        // 160/320点递归最后N=5，part必然为1.非1则出错
        // if (part != 1)
        // {
        //     return;
        // }

        // 分解为5个子序列
        // x[0], x[5], x[10], ...  -> temp[0..part-1]
        // x[1], x[6], x[11], ...  -> temp[part..2*part-1]
        // ...

        for (int p = 0; p < 5; p++)
        {
            for (int i = 0; i < part; i++)
            {
                float r, i_val;
                get_complex(x, (5 * i + p), &r, &i_val);
                set_complex(temp, p * part + i, r, i_val);
            }
            // 递归每个部分
            mixed_radix_2_5_fft_160_320(temp + 2 * p * part, part);
        }

        // 预计算 W5 = exp(-2*pi*i/5) 的幂次
        float w5_r[5], w5_i[5]; // W5^0, W5^1, W5^2, W5^3, W5^4
        for (int m = 0; m < 5; m++)
        {

            w5_r[m] = twiddle_radix_5_5[m][0];
            w5_i[m] = twiddle_radix_5_5[m][1];
        }

        // 合并5个部分
        for (int k = 0; k < part; k++)
        {
            float x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i, x4r, x4i;

            // 读取5个子FFT的结果 X_m(k)
            get_complex(temp, 0, &x0r, &x0i);
            get_complex(temp, part, &x1r, &x1i);
            get_complex(temp, 2 * part, &x2r, &x2i);
            get_complex(temp, 3 * part, &x3r, &x3i);
            get_complex(temp, 4 * part, &x4r, &x4i);

            // 计算 W_N^(m*k) 对于 m=0,1,2,3,4
            // X(k + n*part) = sum_{m=0}^{4} W5^(m*n) * W_N^(m*k) * X_m(k)
            // 其中 n = 0,1,2,3,4

            for (int n = 0; n < 5; n++)
            { // 输出索引偏移 n*part
                float sum_r = 0.0f, sum_i = 0.0f;

                for (int m = 0; m < 5; m++)
                { // 5个部分的累加
                    // W_N^(m*k) = exp(-2*pi*i*m*k/N)
                    // float angle_mk = -2.0f * M_PI * m * k / N;
                    // float wnk_r = cosf(angle_mk);
                    // float wnk_i = sinf(angle_mk);

                    // k这里只能取0，结果固定
                    float wnk_r = 1.0f;
                    float wnk_i = 0.0f;

                    // 先乘 W_N^(m*k) * X_m(k)
                    float wxr, wxi;
                    switch (m)
                    {
                    case 0:
                        complex_multiply(wnk_r, wnk_i, x0r, x0i, &wxr, &wxi);
                        break;
                    case 1:
                        complex_multiply(wnk_r, wnk_i, x1r, x1i, &wxr, &wxi);
                        break;
                    case 2:
                        complex_multiply(wnk_r, wnk_i, x2r, x2i, &wxr, &wxi);
                        break;
                    case 3:
                        complex_multiply(wnk_r, wnk_i, x3r, x3i, &wxr, &wxi);
                        break;
                    case 4:
                        complex_multiply(wnk_r, wnk_i, x4r, x4i, &wxr, &wxi);
                        break;
                    }

                    // 再乘 W5^(m*n)
                    float w5mn_r = w5_r[(m * n) % 5];
                    float w5mn_i = w5_i[(m * n) % 5];

                    float term_r, term_i;
                    complex_multiply(w5mn_r, w5mn_i, wxr, wxi, &term_r, &term_i);

                    // 累加
                    sum_r += term_r;
                    sum_i += term_i;
                }

                set_complex(x, (n * part), sum_r, sum_i);
            }
        }
    }
    else
    {
        print("Error: N=%d 包含非2/5的质因子，只能处理 N=2^n*5^m\n", N);
        return;
    }
}