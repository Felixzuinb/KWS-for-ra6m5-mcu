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
* File Name    : network.c
* Version      : 1.00
* Description  : Definitions of all functions
***********************************************************************************************************************/
/**********************************************************************************************************************
* History : DD.MM.YYYY Version  Description
*         : 16.06.2017 1.00     First Release
***********************************************************************************************************************/

#include "stdlib.h"
#include "Typedef.h"
#include "math.h"
#include "layer_graph.h"
#include "struct_nn.h"
#include "Typedef.h"
#include "arm_nnfunctions.h"
/***********************************************************************************************************************
* Function Name: transpose
* Description  : Performs matrix transpose
* Arguments    : dData		- array of input data
*				 dOut		- placeholder for the output
*				 iShapes	- dimensions of input array + flag to represent transpose axis (i.e., data)
*                errorcode - errorcode if any issue 
* Return Value : no return value
***********************************************************************************************************************/
void transpose4d(TsInt8 *data, TsInt8 *output, TsInt *iShapes, TsInt *errorcode)
{    
    if (*errorcode!=1)
    {
        TsInt flag = iShapes[4];

        // Execute RXDSP or CMSIS Transpose4d or C Transpose 4d or CCRX complier or CCRL complier
        TsInt c = 0;
        TsInt i,j;
        if(flag == 0){      // NHWC to NCHW
            TsInt N = iShapes[0]; // Number of samples
            TsInt H = iShapes[1]; // Number of channels
            TsInt W = iShapes[2]; // Input image height
            TsInt C = iShapes[3]; // Input image width
            
            for(i = 0; i < N*C ; i++){
                for(j = 0; j < H*W ; j++){        
                    output[c] = data[i+(j*C)];
                    c++;
                }
            }

        }
        else if(flag == 1){     // NCHW to NHWC
            TsInt N = iShapes[0]; // Number of samples
            TsInt C = iShapes[1]; // Number of channels
            TsInt H = iShapes[2]; // Input image height
            TsInt W = iShapes[3]; // Input image width

            for(i = 0; i < H*W ; i++){
                for(j = 0; j < N*C ; j++){      
                    output[c] = data[i+(j*H*W)];
                    c++;
                }

            }
        }
    }
    
}



