#include "arm_nn_types.h"

typedef struct 
{
    cmsis_nn_dims input_dims;
	cmsis_nn_dims filter_dims;
	cmsis_nn_dims output_dims;
	cmsis_nn_dims bias_dims;
	cmsis_nn_conv_params conv_params;
}conv;
 
typedef struct 
{
    cmsis_nn_dims input_dims;
	cmsis_nn_dims filter_dims;
	cmsis_nn_dims output_dims;
	cmsis_nn_dims bias_dims;
	cmsis_nn_fc_params fc_params;
}fc;

typedef struct 
{
    cmsis_nn_dims input_dims;
	cmsis_nn_dims filter_dims;
	cmsis_nn_dims output_dims;
	cmsis_nn_dims bias_dims;
	cmsis_nn_dw_conv_params dw_conv_params;
}dconv;

typedef struct 
{
	cmsis_nn_pool_params pool_params;
	cmsis_nn_dims input_dims;
	cmsis_nn_dims filter_dims;
	cmsis_nn_dims output_dims;
}m_pool;

typedef struct 
{
	cmsis_nn_pool_params pool_params;
	cmsis_nn_dims input_dims;
	cmsis_nn_dims filter_dims;
	cmsis_nn_dims output_dims;
}avg_pool;
