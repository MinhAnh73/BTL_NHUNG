#ifndef APP_CFG_H
#define APP_CFG_H
#include "FreeRTOS.h"
#include "task.h"
#define SENSOR_PRIO (tskIDLE_PRIORITY + 3)
#define STORAGE_PRIO (tskIDLE_PRIORITY + 3)
#define DISPLAY_PRIO (tskIDLE_PRIORITY + 2)
#define BUZZER_PRIO (tskIDLE_PRIORITY + 2)
/* Stacks in uint32_t words (Keil FreeRTOS Cortex M3). */
#define SENSOR_STACK_WORDS 400
#define STORAGE_STACK_WORDS 320
#define DISPLAY_STACK_WORDS 400
#define BUZZER_STACK_WORDS 192
#endif
