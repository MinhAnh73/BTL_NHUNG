#ifndef APP_IPC_H
#define APP_IPC_H
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "app_types.h"
extern QueueHandle_t q_hx_samples; /* latest raw HX711 -> QCTask */
extern QueueHandle_t q_display;    /* latest QC state -> OLEDTask */
extern TaskHandle_t buzzer_task_handle;
BaseType_t AppIPC_Init(void);
void AppIPC_Sample(const HxSample *sample);
void AppIPC_Display(const DisplayFrame *frame);
void AppIPC_Beep(AlarmMode mode);
#endif
