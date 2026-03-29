/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_x-cube-ai.c
  * @brief   X-CUBE-AI initialization and inference
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "app_x-cube-ai.h"
#include "gnn100_8_3.h"
#include "gnn100_8_3_data.h"
#include <string.h>

/* Private variables ---------------------------------------------------------*/
static ai_handle gnn100_8_3_handle = AI_HANDLE_NULL;
static ai_buffer ai_input[AI_GNN100_8_3_IN_NUM] = {0};
static ai_buffer ai_output[AI_GNN100_8_3_OUT_NUM] = {0};

/* USER CODE BEGIN PV */
AI_ALIGNED(32) static ai_u8 activations[AI_GNN100_8_3_DATA_ACTIVATIONS_SIZE];
/* USER CODE END PV */

int MX_X_CUBE_AI_Init(void)
{
  ai_error err;
  ai_bool status;
  err = ai_gnn100_8_3_create(&gnn100_8_3_handle, AI_GNN100_8_3_DATA_CONFIG);
  if (err.type != AI_ERROR_NONE) {
    return -1;
  }

  const ai_network_params params = {
    AI_GNN100_8_3_DATA_WEIGHTS(ai_gnn100_8_3_data_weights_get()),
    AI_GNN100_8_3_DATA_ACTIVATIONS(activations)
  };

  status = ai_gnn100_8_3_init(gnn100_8_3_handle, &params);
  if (status != true) {
    err = ai_gnn100_8_3_get_error(gnn100_8_3_handle);
    return -2;
  }

  ai_network_report report;
  status = ai_gnn100_8_3_get_info(gnn100_8_3_handle, &report);
  if (status != true) {
    return -3;
  }

  ai_input[0] = report.inputs[0];
  ai_output[0] = report.outputs[0];

  return 0;
}

int ai_run(const float *input_data, float *output_data)
{
  ai_i32 nbatch;

  ai_input[0].data = AI_HANDLE_PTR((void*)input_data);
  ai_output[0].data = AI_HANDLE_PTR((void*)output_data);

  nbatch = ai_gnn100_8_3_run(gnn100_8_3_handle, &ai_input[0], &ai_output[0]);
  if (nbatch != 1) {
    return -1;
  }

  return 0;
}
