/*
 * HX711Task — chỉ làm 1 việc: đọc ADC và gửi mẫu RAW cho QCTask.
 * Không biết gì về gam, TARE hay PASS/FAIL (những việc đó thuộc QCTask).
 */
#include "task_hx711.h"
#include "app_config.h"
#include "app_ipc.h"
#include "hx711.h"

static void HX711_Task(void *arg)
{
    (void)arg;
    HX711_Init();
    TickType_t wake = xTaskGetTickCount();

    for (;;) {
        int32_t raw;
        if (HX711_Read(&raw)) {                 /* false nếu DOUT chưa sẵn sàng */
            HxSample sample;
            sample.raw          = raw;
            sample.timestamp_ms = HAL_GetTick();
            AppIPC_SendSample(&sample);
        }
        AppIPC_TaskAlive(TASK_ID_HX711);
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(HX_TASK_PERIOD_MS));
    }
}

BaseType_t Task_HX711_Create(void)
{
    return xTaskCreate(HX711_Task, "HX711", STACK_HX711_WORDS, NULL,
                       PRIO_HX711, &g_task_handles[TASK_ID_HX711]);
}
