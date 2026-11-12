#ifndef APP_CONFIG_H
#define APP_CONFIG_H
#include "main.h"
/* All GPIO buttons: pull-up, a press connects to GND. */
#define HX_DOUT_PORT GPIOB
#define HX_DOUT_PIN GPIO_PIN_12
#define HX_SCK_PORT GPIOB
#define HX_SCK_PIN GPIO_PIN_13
#define BTN_PORT GPIOA
#define BTN_TARE_PIN GPIO_PIN_0
#define BTN_MODE_PIN GPIO_PIN_1
#define BTN_UP_PIN GPIO_PIN_2
#define BTN_DOWN_PIN GPIO_PIN_3
#define BTN_RESET_PIN GPIO_PIN_0
#define BTN_RESET_PORT GPIOB
#define BTN_CAL_PIN GPIO_PIN_1
#define BTN_CAL_PORT GPIOB
#define SCALE_CAPACITY_G 5000
#define PHYSICAL_OVERLOAD_G 4900
#define PRESENCE_G 20
#define STABLE_DELTA_G 3
#define STABLE_SAMPLES 4
#define REMOVAL_SAMPLES 3
#define REARM_DELTA_G 15
#define SENSOR_TIMEOUT_MS 800u
/* Only used before calibration; never used for valid QC results. */
#define DEFAULT_COUNTS_PER_GRAM 1000.0f
#define DEFAULT_TARGET_G 500u
#define DEFAULT_TOLERANCE_G 10u
#define CAL_DEFAULT_G 1000u
#define CAL_MIN_G 100u
#define CAL_MAX_G 4000u
#define CAL_STEP_G 5u
#define HX_TASK_POLL_MS 20u
#define QC_TASK_POLL_MS 20u
#define OLED_UPDATE_MS 250u
#endif
