#ifndef APP_CFG_H
#define APP_CFG_H
#include "FreeRTOS.h"
#include "task.h"
#define SENSOR_PRIO  (tskIDLE_PRIORITY + 3)
#define QC_PRIO      (tskIDLE_PRIORITY + 3)
#define DISPLAY_PRIO (tskIDLE_PRIORITY + 2)
#define BUZZER_PRIO  (tskIDLE_PRIORITY + 2)
/* xTaskCreate stack sizes are words, NOT bytes. */
#define SENSOR_STACK_WORDS 256
#define QC_STACK_WORDS 440
#define DISPLAY_STACK_WORDS 420
#define BUZZER_STACK_WORDS 192
#endif
