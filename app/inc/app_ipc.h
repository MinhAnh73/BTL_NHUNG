#ifndef APP_IPC_H
#define APP_IPC_H
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "app_types.h"
extern QueueHandle_t q_display; /* overwrite: latest state */
extern QueueHandle_t q_storage_write; /* overwrite: most recent full snapshot */
extern QueueHandle_t q_storage_boot; /* storage -> HX711 at boot */
extern TaskHandle_t buzzer_task_handle;
BaseType_t AppIPC_Init(void);
void AppIPC_Display(const DisplayFrame *frame);
void AppIPC_Save(const PersistData *data);
void AppIPC_Beep(AlarmMode mode);
#endif
