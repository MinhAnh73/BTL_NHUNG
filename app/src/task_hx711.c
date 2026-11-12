/* Person 1: HX711Task only owns hardware sampling, never QC state. */
#include "task_hx711.h"
#include "app_cfg.h"
#include "app_ipc.h"
#include "hx711.h"
#include "main.h"
#include "app_config.h"
static void Run(void *arg){
 (void)arg;
 HX711_Init();
 TickType_t last=xTaskGetTickCount();
 for(;;){
  int32_t raw;
  if(HX711_IsReady() && HX711_Read(&raw)){
   HxSample sample={raw,HAL_GetTick()};
   AppIPC_Sample(&sample);
  }
  vTaskDelayUntil(&last,pdMS_TO_TICKS(HX_TASK_POLL_MS));
 }
}
BaseType_t Task_HX711_Create(void){
 return xTaskCreate(Run,"HX711",SENSOR_STACK_WORDS,NULL,SENSOR_PRIO,NULL);
}
