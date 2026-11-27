#include "button.h"
#include "app_config.h"
#include <stdbool.h>

enum { BTN_TARE = 0, BTN_MODE, BTN_UP, BTN_DOWN, BTN_RESET, BTN_CAL, BTN_COUNT };

typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
    bool          raw;          /* mức đọc được lần gần nhất (chưa lọc)      */
    bool          stable;       /* mức đã ổn định >= BTN_DEBOUNCE_MS          */
    uint32_t      changed_ms;   /* thời điểm raw thay đổi lần cuối            */
    uint32_t      pressed_ms;   /* thời điểm bắt đầu nhấn (sau debounce)      */
    bool          long_fired;   /* đã phát sự kiện nhấn giữ trong lần nhấn này */
} Button;

static Button buttons[BTN_COUNT] = {
    { .port = BTN_TARE_PORT, .pin = BTN_TARE_PIN },
    { .port = BTN_MODE_PORT, .pin = BTN_MODE_PIN },
    { .port = BTN_UP_PORT, .pin = BTN_UP_PIN },
    { .port = BTN_DOWN_PORT, .pin = BTN_DOWN_PIN },
    { .port = BTN_RESET_PORT, .pin = BTN_RESET_PIN },
    { .port = BTN_CAL_PORT, .pin = BTN_CAL_PIN },
};

static bool is_pressed(const Button *b)
{
    return HAL_GPIO_ReadPin(b->port, b->pin) == GPIO_PIN_RESET;   /* nhấn = mức 0 */
}

void Button_Init(void)
{
    for (unsigned i = 0; i < BTN_COUNT; i++) {
        buttons[i].raw        = false;
        buttons[i].stable     = false;
        buttons[i].changed_ms = 0;
        buttons[i].pressed_ms = 0;
        buttons[i].long_fired = false;
    }
}

uint32_t Button_Poll(uint32_t now_ms)
{
    uint32_t events = 0;

    for (unsigned i = 0; i < BTN_COUNT; i++) {
        Button *b = &buttons[i];

        /* 1. Debounce: mức mới phải giữ nguyên đủ BTN_DEBOUNCE_MS mới được chấp nhận. */
        bool now_pressed = is_pressed(b);
        if (now_pressed != b->raw) {
            b->raw        = now_pressed;
            b->changed_ms = now_ms;
        }
        bool settled = (uint32_t)(now_ms - b->changed_ms) >= BTN_DEBOUNCE_MS;

        if (settled && b->raw != b->stable) {
            b->stable = b->raw;

            if (b->stable) {                        /* ---- vừa nhấn xuống ---- */
                b->pressed_ms = now_ms;
                b->long_fired = false;
                if (i <= BTN_DOWN) {
                    events |= (1u << i);            /* TARE/MODE/UP/DOWN: tác động ngay */
                }
            } else {                                /* ---- vừa nhả ra ---- */
                if (i == BTN_CAL && !b->long_fired) {
                    events |= BUTTON_CAL_SHORT;     /* CAL nhấn nhanh: tính khi nhả */
                }
            }
        }

        /* 2. Nhấn giữ: chỉ RESET và CAL có chức năng giữ lâu. */
        bool held_long = b->stable && !b->long_fired &&
                         (uint32_t)(now_ms - b->pressed_ms) >= BTN_LONG_MS;
        if (held_long && (i == BTN_RESET || i == BTN_CAL)) {
            b->long_fired = true;
            events |= (i == BTN_RESET) ? BUTTON_RESET_LONG : BUTTON_CAL_LONG;
        }
    }
    return events;
}

uint32_t Button_RawMask(void)
{
    uint32_t mask = 0;
    for (unsigned i = 0; i < BTN_COUNT; i++) {
        if (is_pressed(&buttons[i])) {
            mask |= (1u << i);
        }
    }
    return mask;
}
