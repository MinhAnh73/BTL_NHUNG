#include "state_machine.h"
#include "app_config.h"
static int32_t abs32(int32_t v){return v<0?-v:v;}
void QC_Init(QcMachine *m){
 m->state=QC_INIT;m->previous_g=0;m->locked_g=0;
 m->stable_samples=0;m->absent_samples=0;m->verdict=-1;
}
QcEvent QC_Update(QcMachine *m,int32_t grams,int32_t gross_grams,
                  bool sensor_ok,uint32_t target_g,uint32_t tolerance_g){
 QcEvent e={false,false,0};
 if(!sensor_ok){m->state=QC_ERROR;m->stable_samples=0;m->absent_samples=0;return e;}
 if(abs32(gross_grams)>PHYSICAL_OVERLOAD_G){
  m->state=QC_OVERLOAD;m->stable_samples=0;m->absent_samples=0;return e;
 }
 if(m->state==QC_ERROR||m->state==QC_OVERLOAD||m->state==QC_INIT){
  m->state=QC_IDLE;m->verdict=-1;m->stable_samples=0;m->absent_samples=0;
 }
 /* A completed measurement is recorded only after the item is removed. */
 if(m->state==QC_WAIT_REMOVE||m->state==QC_PASS||m->state==QC_FAIL){
  if(grams<PRESENCE_G){
   if(++m->absent_samples>=REMOVAL_SAMPLES){
    e.completed=true;e.completed_g=m->locked_g;e.completed_pass=(m->verdict==1);
    m->state=QC_IDLE;m->verdict=-1;m->stable_samples=0;m->absent_samples=0;
   } else m->state=QC_WAIT_REMOVE;
  } else if(grams>m->locked_g+REARM_DELTA_G){
   /* Additional material: re-measure. Falling EMA values during item removal
      must NOT cancel the locked result. */
   m->state=QC_MEASURING;m->stable_samples=0;m->absent_samples=0;m->verdict=-1;
  } else m->state=QC_WAIT_REMOVE;
  m->previous_g=grams;return e;
 }
 if(grams<PRESENCE_G){
  m->state=QC_IDLE;m->stable_samples=0;m->previous_g=grams;return e;
 }
 if(m->state==QC_IDLE){m->stable_samples=0;m->state=QC_MEASURING;}
 if(abs32(grams-m->previous_g)<=STABLE_DELTA_G){
  if(m->stable_samples<STABLE_SAMPLES)m->stable_samples++;
 } else m->stable_samples=0;
 if(m->stable_samples>=STABLE_SAMPLES){
  m->state=QC_STABLE;
  m->locked_g=grams;
  m->verdict=((int64_t)grams >= (int64_t)target_g-(int64_t)tolerance_g &&
              (int64_t)grams <= (int64_t)target_g+(int64_t)tolerance_g)?1:0;
  m->state=(m->verdict==1)?QC_PASS:QC_FAIL;
 } else m->state=QC_MEASURING;
 m->previous_g=grams;return e;
}
const char *QC_StateName(QcState s){
 switch(s){
 case QC_INIT:return "INIT";case QC_IDLE:return "IDLE";
 case QC_MEASURING:return "MEASURING";case QC_STABLE:return "STABLE";
 case QC_PASS:return "PASS";case QC_FAIL:return "FAIL";
 case QC_WAIT_REMOVE:return "REMOVE ITEM";case QC_OVERLOAD:return "OVERLOAD";
 case QC_ERROR:return "SENSOR ERROR";default:return "UNKNOWN";
 }
}
