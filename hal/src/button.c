#include "button.h"
#include "app_config.h"
#include "main.h"
#include <stdbool.h>
typedef struct {GPIO_TypeDef *port;uint16_t pin;uint8_t stable,raw;uint32_t changed_ms,down_ms;bool long_fired;} Btn;
static Btn btn[6];
void Button_Init(void){
 const uint16_t pins[6]={BTN_TARE_PIN,BTN_MODE_PIN,BTN_UP_PIN,BTN_DOWN_PIN,BTN_RESET_PIN,BTN_CAL_PIN};
 for(unsigned i=0;i<6;i++){
  btn[i].port=i<4?BTN_PORT:GPIOB;
  btn[i].pin=pins[i];btn[i].raw=0;btn[i].stable=0;
  btn[i].changed_ms=0;btn[i].down_ms=0;btn[i].long_fired=false;
 }
}
uint32_t Button_Poll(uint32_t ms){
 uint32_t events=0;
 for(unsigned i=0;i<6;i++){
  Btn *b=&btn[i];
  uint8_t pressed=HAL_GPIO_ReadPin(b->port,b->pin)==GPIO_PIN_RESET;
  if(pressed!=b->raw){b->raw=pressed;b->changed_ms=ms;}
  if(b->raw!=b->stable && (uint32_t)(ms-b->changed_ms)>=35u){
   b->stable=b->raw;
   if(b->stable){b->down_ms=ms;b->long_fired=false;}
   else if(i<4 && !b->long_fired)events|=(1u<<i);
   else if(i==5 && !b->long_fired)events|=BUTTON_CAL_SHORT;
  }
  if(i>=4 && b->stable && !b->long_fired && (uint32_t)(ms-b->down_ms)>=2000u){
   b->long_fired=true;events|=(i==4?BUTTON_RESET_LONG:BUTTON_CAL_LONG);
  }
 }
 return events;
}
