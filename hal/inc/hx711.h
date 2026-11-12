#ifndef HX711_H
#define HX711_H
#include "main.h"
#include <stdbool.h>
#include <stdint.h>
void HX711_Init(void);
bool HX711_IsReady(void);
bool HX711_Read(int32_t *value);
#endif
