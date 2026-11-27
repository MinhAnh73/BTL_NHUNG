#ifndef APP_IPC_H
#define APP_IPC_H
/*
 * Giao tiếp giữa các task (IPC) và giám sát sức khỏe task.
 *
 *   HX711Task --q_hx_samples (Queue 1 phần tử, ghi đè)--> QCTask
 *   QCTask    --q_display    (Queue 1 phần tử, ghi đè)--> OLEDTask
 *   QCTask    --Task Notification (giá trị = AlarmMode)--> BuzzerTask
 *
 * Queue 1 phần tử + xQueueOverwrite = "hộp thư": luôn giữ GIÁ TRỊ MỚI NHẤT,
 * bên gửi không bao giờ bị chặn, bên nhận không bao giờ đọc dữ liệu cũ.
 */
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "app_types.h"

typedef enum {
    TASK_ID_HX711 = 0,
    TASK_ID_QC,
    TASK_ID_OLED,
    TASK_ID_BUZZER,
    TASK_COUNT
} TaskId;

extern QueueHandle_t q_hx_samples;
extern QueueHandle_t q_display;
extern TaskHandle_t  g_task_handles[TASK_COUNT];

/* ---- Thông tin giám sát: mở Keil Watch để xem khi chạy debug ---- */
extern volatile uint32_t g_stack_free_words[TASK_COUNT];  /* stack còn trống ít nhất (word) */
extern volatile uint32_t g_heap_min_free_bytes;           /* heap còn trống ít nhất (byte)  */

BaseType_t AppIPC_Init(void);

void AppIPC_SendSample(const HxSample *sample);
void AppIPC_SendDisplay(const DisplayFrame *frame);
void AppIPC_Beep(AlarmMode mode);

/* Mỗi task gọi 1 lần trong mỗi vòng lặp để báo "tôi còn chạy". */
void AppIPC_TaskAlive(TaskId id);

/* QCTask gọi mỗi HEALTH_CHECK_MS: trả về true nếu MỌI task đều đã báo còn chạy
   kể từ lần kiểm tra trước. Đồng thời cập nhật g_stack_free_words / g_heap_min_free_bytes. */
bool AppIPC_CheckHealth(void);

#endif /* APP_IPC_H */
