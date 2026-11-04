#include "app_ipc.h"
QueueHandle_t q_display;
QueueHandle_t q_storage_write;
QueueHandle_t q_storage_boot;
TaskHandle_t buzzer_task_handle;
BaseType_t AppIPC_Init(void){
 q_display=xQueueCreate(1,sizeof(DisplayFrame));
 q_storage_write=xQueueCreate(1,sizeof(PersistData));
 q_storage_boot=xQueueCreate(1,sizeof(StorageBoot));
 return (q_display && q_storage_write && q_storage_boot)?pdPASS:pdFAIL;
}
void AppIPC_Display(const DisplayFrame *frame){if(frame)xQueueOverwrite(q_display,frame);}
void AppIPC_Save(const PersistData *data){if(data)xQueueOverwrite(q_storage_write,data);}
void AppIPC_Beep(AlarmMode mode){
 if(buzzer_task_handle)xTaskNotify(buzzer_task_handle,(uint32_t)mode,eSetValueWithOverwrite);
}
