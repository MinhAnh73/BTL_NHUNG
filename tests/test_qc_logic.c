/* Host-side unit tests for PASS/FAIL, removal, overload and anti-double-count. */
#include <assert.h>
#include <stdio.h>
#include "state_machine.h"
static QcEvent step(QcMachine *s,int32_t mass){return QC_Update(s,mass,mass,true,500,10);}
static void stable(QcMachine *s,int32_t mass){for(int i=0;i<7;i++)(void)step(s,mass);}
int main(void){
 QcMachine s;QC_Init(&s);
 stable(&s,0);assert(s.state==QC_IDLE);
 stable(&s,480);assert(s.state==QC_FAIL || s.state==QC_WAIT_REMOVE);
 assert(!step(&s,0).completed);assert(!step(&s,0).completed);
 QcEvent e=step(&s,0);assert(e.completed&&!e.completed_pass&&e.completed_g==480);
 assert(!step(&s,0).completed);
 stable(&s,498);assert(s.state==QC_PASS || s.state==QC_WAIT_REMOVE);
 for(int i=0;i<20;i++)assert(!step(&s,498).completed);
 /* EMA decay when removing the item: 498 -> 324 -> 211 ... -> 0. */
 int32_t falling[]={324,211,137,89,58,38,25,16,10,4};
 int recorded=0;
 for(unsigned i=0;i<sizeof(falling)/sizeof(falling[0]);i++){
  e=step(&s,falling[i]);
  if(e.completed){assert(e.completed_pass&&e.completed_g==498);recorded++;}
 }
 assert(recorded==1);
 assert(!step(&s,0).completed);
 stable(&s,430);assert(s.state==QC_FAIL || s.state==QC_WAIT_REMOVE);
 (void)step(&s,500);assert(s.state==QC_MEASURING); /* more material triggers rearm */
 stable(&s,500);assert(s.state==QC_PASS || s.state==QC_WAIT_REMOVE);
 (void)QC_Update(&s,0,5000,true,500,10);assert(s.state==QC_OVERLOAD);
 (void)QC_Update(&s,0,0,false,500,10);assert(s.state==QC_ERROR);
 stable(&s,0);assert(s.state==QC_IDLE);
 puts("QC state-machine tests: PASS");return 0;
}
