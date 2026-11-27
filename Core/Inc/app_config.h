#ifndef APP_CONFIG_H
#define APP_CONFIG_H
/*
 * Cấu hình PHẦN CỨNG và RTOS: chân GPIO, tốc độ bus, ưu tiên và stack của task.
 * Tham số logic (gam, số mẫu, thời gian) nằm trong app_params.h.
 */
#include "main.h"
#include "app_params.h"

/* ---------------- Chân GPIO (xem bảng nối dây trong README) ---------------- */
#define HX_DOUT_PORT        GPIOB
#define HX_DOUT_PIN         GPIO_PIN_12
#define HX_SCK_PORT         GPIOB
#define HX_SCK_PIN          GPIO_PIN_13

/* Nút nhấn: nối xuống GND, dùng pull-up nội => nhấn = mức 0 */
#define BTN_TARE_PORT       GPIOA
#define BTN_TARE_PIN        GPIO_PIN_0
#define BTN_MODE_PORT       GPIOA
#define BTN_MODE_PIN        GPIO_PIN_1
#define BTN_UP_PORT         GPIOA
#define BTN_UP_PIN          GPIO_PIN_2
#define BTN_DOWN_PORT       GPIOA
#define BTN_DOWN_PIN        GPIO_PIN_3
#define BTN_RESET_PORT      GPIOB
#define BTN_RESET_PIN       GPIO_PIN_0
#define BTN_CAL_PORT        GPIOB
#define BTN_CAL_PIN         GPIO_PIN_1

/* Buzzer thụ động: TIM1_CH1 trên PA8. 72 MHz / (71+1) / (499+1) = 2 kHz */
#define BUZZER_TIM_PRESCALER    71u
#define BUZZER_TIM_PERIOD       499u

/* OLED SSD1306 trên I2C1 (PB6 = SCL, PB7 = SDA) */
#define OLED_I2C_ADDR       (0x3Cu << 1)    /* HAL dùng địa chỉ 8 bit */
#define OLED_I2C_CLOCK_HZ   400000u         /* Fast mode; hạ về 100000 nếu dây dài */

/* ---------------- Task FreeRTOS ---------------- */
/* Ưu tiên: số lớn = ưu tiên cao. Idle task = 0.
   Task đo và xử lý QC quan trọng hơn task giao diện. */
#define PRIO_HX711          3
#define PRIO_QC             3
#define PRIO_OLED           2
#define PRIO_BUZZER         2

/* Kích thước stack tính bằng WORD (1 word = 4 byte).
   Xem mức stack còn trống thực tế trong biến g_stack_free_words (Keil Watch). */
#define STACK_HX711_WORDS   256
#define STACK_QC_WORDS      440
#define STACK_OLED_WORDS    420
#define STACK_BUZZER_WORDS  192

#endif /* APP_CONFIG_H */
