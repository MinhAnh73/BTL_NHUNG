#ifndef TASK_HX711_H
#define TASK_HX711_H
/* Đọc HX711, gửi mẫu RAW cho QCTask. */
#include "FreeRTOS.h"
#include "task.h"

BaseType_t Task_HX711_Create(void);

#endif
