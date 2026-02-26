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

#include "Typedef.h"
#include <stdlib.h>
#ifndef LAYER_SHAPES_H_
#define LAYER_SHAPES_H_
 
TsOUT* dnn_compute(TsIN*, TsInt*);
 
TsInt8 dnn_buffer1[40000];
TsInt8 dnn_buffer2[16000];
TsOUT StatefulPartitionedCall_0[5];
 
struct shapes{
    TFloat tfl_quantize_shape[3];
    TsInt sequential_conv2d_Relu_shape[16];
    TsInt sequential_max_pooling2d_MaxPool_shape[11];
    TsInt sequential_conv2d_1_Relu_shape[16];
    TsInt sequential_max_pooling2d_1_MaxPool_shape[11];
    TsInt sequential_max_pooling2d_1_MaxPool_tr_shape[5];
    TsInt sequential_dense_MatMul_shape[7];
    TsInt sequential_dense_1_MatMul_shape[7];
    TsInt StatefulPartitionedCall_01_shape;
    TFloat StatefulPartitionedCall_0_shape[3];
};
 
struct shapes layer_shapes ={
    {4000,0.0015694326721131802,-128},
    {1,1,50,80,10,1,3,3,50,80,1,1,1,1,1,1},
    {1,10,50,80,25,40,2,2,2,2,0},
    {1,10,25,40,16,10,3,3,25,40,1,1,1,1,1,1},
    {1,16,25,40,12,20,2,2,2,2,0},
    {1,16,12,20,1},
    {1,3840,3840,16,1,1,0},
    {1,16,16,5,1,1,0},
    5,
    {5,0.00390625,-128}
};
 
#endif
