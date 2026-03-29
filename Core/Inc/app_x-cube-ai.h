/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_x-cube-ai.h
  * @brief   Header for X-CUBE-AI
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef APP_X_CUBE_AI_H
#define APP_X_CUBE_AI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Function prototypes -------------------------------------------------------*/
int MX_X_CUBE_AI_Init(void);
int ai_run(const float *input_data, float *output_data);

#ifdef __cplusplus
}
#endif

#endif /* APP_X_CUBE_AI_H */
