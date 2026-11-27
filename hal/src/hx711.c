#include "hx711.h"
#include "app_config.h"
#include "FreeRTOS.h"
#include "task.h"

/* HX711 yêu cầu: SCK mức cao >= 0.2 us và KHÔNG quá 50 us (quá 60 us => power-down).
   Vòng lặp dưới đây ~1 us ở 72 MHz. Không cần chính xác, chỉ cần nằm trong khoảng trên.
   (Không dùng DWT->CYCCNT vì một số chip clone trên Blue Pill không có bộ đếm này.) */
#define HX711_DELAY_LOOPS  10u

static void short_delay(void)
{
    for (volatile uint32_t i = 0; i < HX711_DELAY_LOOPS; i++) {
        /* chờ */
    }
}

static void sck_pulse(void)
{
    HAL_GPIO_WritePin(HX_SCK_PORT, HX_SCK_PIN, GPIO_PIN_SET);
    short_delay();
    HAL_GPIO_WritePin(HX_SCK_PORT, HX_SCK_PIN, GPIO_PIN_RESET);
    short_delay();
}

void HX711_Init(void)
{
    HAL_GPIO_WritePin(HX_SCK_PORT, HX_SCK_PIN, GPIO_PIN_RESET);
}

bool HX711_IsReady(void)
{
    return HAL_GPIO_ReadPin(HX_DOUT_PORT, HX_DOUT_PIN) == GPIO_PIN_RESET;
}

bool HX711_Read(int32_t *raw)
{
    uint32_t bits = 0;

    if (raw == 0 || !HX711_IsReady()) {
        return false;
    }

    /* Critical section (~75 us): nếu bị task khác chen vào giữa lúc SCK = 1
       quá 60 us, HX711 sẽ tắt => mất mẫu. */
    taskENTER_CRITICAL();
    for (unsigned i = 0; i < 24u; i++) {
        HAL_GPIO_WritePin(HX_SCK_PORT, HX_SCK_PIN, GPIO_PIN_SET);
        short_delay();
        bits <<= 1;
        if (HAL_GPIO_ReadPin(HX_DOUT_PORT, HX_DOUT_PIN) == GPIO_PIN_SET) {
            bits |= 1u;
        }
        HAL_GPIO_WritePin(HX_SCK_PORT, HX_SCK_PIN, GPIO_PIN_RESET);
        short_delay();
    }
    sck_pulse();   /* xung thứ 25: chọn kênh A, gain 128 cho lần đo kế tiếp */
    taskEXIT_CRITICAL();

    /* 0x7FFFFF / 0x800000 = ADC bão hòa (loadcell đứt dây / nối sai).
       Bỏ mẫu này; QCTask sẽ báo SENSOR ERROR sau SENSOR_TIMEOUT_MS. */
    if (bits == 0x7FFFFFu || bits == 0x800000u) {
        return false;
    }

    /* Số bù 2 24 bit -> int32: nếu bit 23 = 1 thì điền 1 vào 8 bit cao. */
    if (bits & 0x800000u) {
        bits |= 0xFF000000u;
    }
    *raw = (int32_t)bits;
    return true;
}
