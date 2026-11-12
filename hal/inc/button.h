#ifndef BUTTON_H
#define BUTTON_H
#include <stdint.h>
#define BUTTON_TARE  (1u<<0)
#define BUTTON_MODE  (1u<<1)
#define BUTTON_UP    (1u<<2)
#define BUTTON_DOWN  (1u<<3)
#define BUTTON_RESET_LONG (1u<<4)
 #define BUTTON_CAL_SHORT (1u<<5)
#define BUTTON_CAL_LONG (1u<<6)
void Button_Init(void);
/* Call from HX711Task every 20ms; millis from HAL_GetTick. */
uint32_t Button_Poll(uint32_t millis);
#endif
