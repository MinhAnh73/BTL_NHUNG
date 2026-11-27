#ifndef BUTTON_H
#define BUTTON_H
/*
 * Driver 6 nút nhấn: chống dội (debounce) và nhận nhấn giữ (long press).
 * Không dùng ngắt: Button_Poll() được QCTask gọi mỗi 20 ms.
 */
#include <stdint.h>

/* Các "sự kiện" trả về từ Button_Poll() — mỗi sự kiện 1 bit, có thể OR nhiều bit. */
#define BUTTON_TARE        (1u << 0)   /* tác động ngay khi nhấn  */
#define BUTTON_MODE        (1u << 1)   /* tác động ngay khi nhấn  */
#define BUTTON_UP          (1u << 2)   /* tác động ngay khi nhấn  */
#define BUTTON_DOWN        (1u << 3)   /* tác động ngay khi nhấn  */
#define BUTTON_RESET_LONG  (1u << 4)   /* giữ RESET >= BTN_LONG_MS */
#define BUTTON_CAL_SHORT   (1u << 5)   /* nhấn nhả CAL nhanh       */
#define BUTTON_CAL_LONG    (1u << 6)   /* giữ CAL >= BTN_LONG_MS   */

void Button_Init(void);

/* Gọi định kỳ; now_ms = HAL_GetTick(). Trả về tập sự kiện mới xảy ra. */
uint32_t Button_Poll(uint32_t now_ms);

/* Trạng thái tức thời của chân (chưa debounce), dùng để chẩn đoán trên OLED.
   bit 0..5 = TARE, MODE, UP, DOWN, RESET, CAL; 1 = đang nhấn. */
uint32_t Button_RawMask(void);

#endif /* BUTTON_H */
