#include "app_ipc.h"
QueueHandle_t q_hx_samples;
QueueHandle_t q_display;
TaskHandle_t buzzer_task_handle;
BaseType_t AppIPC_Init(void){
 q_hx_samples=xQueueCreate(1,sizeof(HxSample));
 q_display=xQueueCreate(1,sizeof(DisplayFrame));
 return (q_hx_samples && q_display)?pdPASS:pdFAIL;
}
void AppIPC_Sample(const HxSample *s){if(s) (void)xQueueOverwrite(q_hx_samples,s);}
void AppIPC_Display(const DisplayFrame *f){if(f) (void)xQueueOverwrite(q_display,f);}
void AppIPC_Beep(AlarmMode mode){
 if(buzzer_task_handle) (void)xTaskNotify(buzzer_task_handle,(uint32_t)mode,eSetValueWithOverwrite);
}
