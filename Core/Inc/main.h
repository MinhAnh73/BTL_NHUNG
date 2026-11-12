#ifndef MAIN_H
#define MAIN_H
#include "stm32f1xx_hal.h"
#include <stdint.h>
extern I2C_HandleTypeDef hi2c1;
extern TIM_HandleTypeDef htim1;
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);
void Error_Handler(void);
#endif
