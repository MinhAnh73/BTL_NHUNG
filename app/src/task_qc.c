/* Person 1: QCTask owns all measurement conversion, buttons and QC state.
   No EEPROM and no fake grams before actual calibration. */
#include "task_qc.h"
#include "app_cfg.h"
#include "app_ipc.h"
#include "app_config.h"
#include "button.h"
#include "scale.h"
#include "state_machine.h"
#include "main.h"
#include <string.h>
static int32_t abs32(int32_t v){return v<0?-v:v;}
static void RenderFrame(const QcSettings *d,const QcMachine *fsm,
                        int32_t grams,ViewMode view,uint32_t cal_mass,bool ok){
 DisplayFrame f;
 memset(&f,0,sizeof f);
 f.grams=grams;f.pass_count=d->pass_count;f.target_g=d->target_g;
 f.tolerance_g=d->tolerance_g;f.cal_mass_g=cal_mass;
 f.calibrated=d->calibrated;f.sensor_ok=ok;
 f.state=fsm->state;f.view=view;f.verdict=fsm->verdict;
 AppIPC_Display(&f);
}
static void Run(void *arg){
 (void)arg;
 QcSettings settings={0,DEFAULT_TARGET_G,DEFAULT_TOLERANCE_G,0};
 Scale_t scale;
 QcMachine fsm;QC_Init(&fsm);
 Button_Init();
 bool have_raw=false,waiting_remove_after_cal=false,zero_captured=false;
 ViewMode view=VIEW_LIVE;
 uint32_t cal_mass=CAL_DEFAULT_G,last_raw_ms=0;
 int32_t raw=0,grams=0,gross=0;
 AlarmMode last_fault=ALARM_SILENT;
 TickType_t wake=xTaskGetTickCount();
 for(;;){
  uint32_t ms=HAL_GetTick();
  HxSample sample;
  bool fresh=xQueueReceive(q_hx_samples,&sample,0)==pdTRUE;
  if(fresh){
   raw=sample.raw;last_raw_ms=sample.timestamp_ms;
   if(!have_raw){Scale_Init(&scale,raw,DEFAULT_COUNTS_PER_GRAM);have_raw=true;}
   /* Pre-CAL conversion is internal only. Never show as a valid gram reading. */
   grams=Scale_Convert(&scale,raw);
   gross=Scale_Gross(&scale,raw);
  }
  bool sensor_ok=have_raw && (uint32_t)(ms-last_raw_ms)<=SENSOR_TIMEOUT_MS;
  uint32_t keys=Button_Poll(ms);
  if(keys&BUTTON_MODE){
   if(view==VIEW_CAL_SET_MASS||view==VIEW_CAL_ZERO||view==VIEW_CAL_WEIGHT){
    view=VIEW_LIVE;zero_captured=false;
   }else view=(view==VIEW_LIVE)?VIEW_SET_TARGET:(view==VIEW_SET_TARGET?VIEW_SET_TOL:VIEW_LIVE);
   QC_Init(&fsm);
  }
  if((keys&(BUTTON_UP|BUTTON_DOWN)) && (view==VIEW_SET_TARGET||view==VIEW_SET_TOL)){
   int32_t delta=(keys&BUTTON_UP)?1:-1;
   if(view==VIEW_SET_TARGET){
    int32_t n=(int32_t)settings.target_g+50*delta;
    if(n>=50&&n<=4500)settings.target_g=(uint32_t)n;
   }else{
    int32_t n=(int32_t)settings.tolerance_g+5*delta;
    if(n>=5&&n<=500)settings.tolerance_g=(uint32_t)n;
   }
   QC_Init(&fsm);
  }
  if((keys&(BUTTON_UP|BUTTON_DOWN)) && view==VIEW_CAL_SET_MASS){
   int32_t n=(int32_t)cal_mass+((keys&BUTTON_UP)?(int32_t)CAL_STEP_G:-(int32_t)CAL_STEP_G);
   if(n>=(int32_t)CAL_MIN_G&&n<=(int32_t)CAL_MAX_G)cal_mass=(uint32_t)n;
  }
  if((keys&BUTTON_CAL_LONG)&&sensor_ok){
   view=VIEW_CAL_SET_MASS;zero_captured=false;QC_Init(&fsm);
  }
  if((keys&BUTTON_CAL_SHORT)&&view==VIEW_CAL_SET_MASS){view=VIEW_CAL_ZERO;}
  if((keys&BUTTON_TARE)&&sensor_ok){
   if(view==VIEW_CAL_ZERO){
    Scale_Tare(&scale,raw);zero_captured=true;
    view=VIEW_CAL_WEIGHT;QC_Init(&fsm);
   }else if(view==VIEW_LIVE){
    Scale_Tare(&scale,raw);grams=0;gross=Scale_Gross(&scale,raw);
    waiting_remove_after_cal=false;QC_Init(&fsm);
   }
  }
  if((keys&BUTTON_CAL_SHORT)&&view==VIEW_CAL_WEIGHT&&zero_captured&&sensor_ok){
   if(Scale_CalibrateKnownMass(&scale,raw,cal_mass)){
    settings.calibrated=1;
    grams=Scale_Convert(&scale,raw);gross=Scale_Gross(&scale,raw);
    waiting_remove_after_cal=true;view=VIEW_LIVE;QC_Init(&fsm);
   }
  }
  if((keys&BUTTON_RESET_LONG)&&view==VIEW_LIVE&&fsm.state==QC_IDLE){
   settings.pass_count=0;
  }
  if(!sensor_ok){
   if(have_raw){(void)QC_Update(&fsm,grams,gross,false,settings.target_g,settings.tolerance_g);}
  } else if(fresh&&view==VIEW_LIVE&&settings.calibrated){
   if(waiting_remove_after_cal){
    QC_Init(&fsm);
    if(abs32(grams)<PRESENCE_G) waiting_remove_after_cal=false;
   }else{
    QcState before=fsm.state;
    QcEvent e=QC_Update(&fsm,grams,gross,true,settings.target_g,settings.tolerance_g);
    if(e.completed&&e.completed_pass&&settings.pass_count!=0xFFFFFFFFu)settings.pass_count++;
    if((fsm.state==QC_PASS||fsm.state==QC_FAIL) &&
       before!=QC_PASS&&before!=QC_FAIL){
      AppIPC_Beep(fsm.state==QC_PASS?ALARM_PASS:ALARM_FAIL);
    }
   }
  }else if(view!=VIEW_LIVE||!settings.calibrated){
   QC_Init(&fsm);
  }
  AlarmMode fault=ALARM_SILENT;
  if(fsm.state==QC_OVERLOAD)fault=ALARM_OVERLOAD;
  else if(fsm.state==QC_ERROR)fault=ALARM_ERROR;
  if(fault!=last_fault){
   if(fault!=ALARM_SILENT)AppIPC_Beep(fault);
   else if(last_fault!=ALARM_SILENT)AppIPC_Beep(ALARM_SILENT);
   last_fault=fault;
  }
  int32_t show_g=(fsm.state==QC_PASS||fsm.state==QC_FAIL||fsm.state==QC_WAIT_REMOVE)
                  ?fsm.locked_g:grams;
  RenderFrame(&settings,&fsm,show_g,view,cal_mass,sensor_ok);
  vTaskDelayUntil(&wake,pdMS_TO_TICKS(QC_TASK_POLL_MS));
 }
}
BaseType_t Task_QC_Create(void){return xTaskCreate(Run,"QC",QC_STACK_WORDS,NULL,QC_PRIO,NULL);}
