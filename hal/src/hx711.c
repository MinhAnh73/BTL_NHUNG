#include "hx711.h"
#include "app_config.h"
#include "FreeRTOS.h"
#include "task.h"
/* The HX711 powers down if SCK is held high >60us.
   The transfer is kept atomic. DWT enables short pulse timing at 72MHz. */
static void spin_cycles(uint32_t n) {
    uint32_t start = DWT->CYCCNT;
    while ((uint32_t)(DWT->CYCCNT - start) < n) { __NOP(); }
}
void HX711_Init(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    HAL_GPIO_WritePin(HX_SCK_PORT,HX_SCK_PIN,GPIO_PIN_RESET);
}
bool HX711_IsReady(void) {
    return HAL_GPIO_ReadPin(HX_DOUT_PORT,HX_DOUT_PIN)==GPIO_PIN_RESET;
}
bool HX711_Read(int32_t *value) {
    if (!value || !HX711_IsReady()) return false;
    uint32_t bits=0;
    uint32_t cycles = SystemCoreClock / 1000000u;
    if (cycles < 1) cycles=1;
    taskENTER_CRITICAL();
    for (unsigned i=0;i<24;i++) {
        HAL_GPIO_WritePin(HX_SCK_PORT,HX_SCK_PIN,GPIO_PIN_SET);
        spin_cycles(cycles * 2);
        bits=(bits<<1) | (HAL_GPIO_ReadPin(HX_DOUT_PORT,HX_DOUT_PIN)==GPIO_PIN_SET);
        HAL_GPIO_WritePin(HX_SCK_PORT,HX_SCK_PIN,GPIO_PIN_RESET);
        spin_cycles(cycles * 2);
    }
    /* 25th pulse: Channel A, gain 128. */
    HAL_GPIO_WritePin(HX_SCK_PORT,HX_SCK_PIN,GPIO_PIN_SET);
    spin_cycles(cycles * 2);
    HAL_GPIO_WritePin(HX_SCK_PORT,HX_SCK_PIN,GPIO_PIN_RESET);
    taskEXIT_CRITICAL();
    if(bits & 0x800000u) bits |= 0xFF000000u;
    *value=(int32_t)bits;
    return true;
}
