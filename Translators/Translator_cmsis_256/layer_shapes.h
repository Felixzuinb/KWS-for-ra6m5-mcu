/***********************************************************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No 
* other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all 
* applicable laws, including copyright laws. 
* THIS SOFTWARE IS PROVIDED 'AS IS' AND RENESAS MAKES NO WARRANTIES REGARDING
* THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, 
* FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM
* EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES 
* SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS 
* SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of 
* this software. By using this software, you agree to the additional terms and conditions found by accessing the 
* following link:
* http://www.renesas.com/disclaimer 
*
* Changed from original python code to C source code.
* Copyright (C) 2017 Renesas Electronics Corporation. All rights reserved.
***********************************************************************************************************************/
/***********************************************************************************************************************
* File Name    : layer_shapes.h
* Version      : 1.00
* Description  : Initializations
***********************************************************************************************************************/
/**********************************************************************************************************************
* History : DD.MM.YYYY Version  Description
*         : 16.06.2017 1.00     First Release
***********************************************************************************************************************/

#include "struct_nn.h"
#include "Typedef.h"
#include <stdlib.h>
#ifndef LAYER_SHAPES_H_
#define LAYER_SHAPES_H_
 
TsOUT* dnn_compute(TsIN*, TsInt*);
 
TsInt8 dnn_buffer1[39680];
TsInt8 dnn_buffer2[39680];
TsOUT StatefulPartitionedCall_0[5];
#if defined(ARM_MATH_MVEI)
int8_t scratch_buffer[1792];
const cmsis_nn_context cr_buffer={(int8_t *)scratch_buffer,1792};
#else
int8_t scratch_buffer[368];
const cmsis_nn_context cr_buffer={(int8_t *)scratch_buffer,368};
#endif
 
struct shapes{
    TFloat tfl_quantize_shape[3];
    TsInt tfl_quantize_tr_shape[5];
    conv sequential_conv2d_Relu_shape;
    m_pool sequential_max_pooling2d_MaxPool_shape;
    conv sequential_conv2d_1_Relu_shape;
    m_pool sequential_max_pooling2d_1_MaxPool_shape;
    fc sequential_dense_MatMul_shape;
    fc sequential_dense_1_MatMul_shape;
    TsInt StatefulPartitionedCall_01_shape;
    TFloat StatefulPartitionedCall_0_shape[3];
};
 
struct shapes layer_shapes ={
    {3968,0.001750428811646998,-128},
    {1,1,62,64,1},
    {{1,62,64,1},{10,3,3,1},{1,62,64,10},{1,1,1,10},{128,-128,{1,1},{1,1},{1,1},{-128,127}}},
    {{{2,2},{0,0},{-128,127}},{1,62,64,10},{1,2,2,1},{1,31,32,10}},
    {{1,31,32,10},{16,3,3,10},{1,31,32,16},{1,1,1,16},{128,-128,{1,1},{1,1},{1,1},{-128,127}}},
    {{{2,2},{0,0},{-128,127}},{1,31,32,16},{1,2,2,1},{1,15,16,16}},
    {{1,1,3840,1},{3840,1,1,16},{1,1,1,16},{1,1,1,16},{128,0,-128,{-128,127}}},
    {{1,1,16,1},{16,1,1,5},{1,1,1,5},{1,1,1,5},{128,0,38,{-128,127}}},
    5,
    {5,0.00390625,-128}
};
 
#endif
