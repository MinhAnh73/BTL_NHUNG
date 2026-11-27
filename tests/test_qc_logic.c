/* Test máy trạng thái QC trên PC. QC_Update() được gọi 1 lần cho mỗi mẫu MỚI. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "scale.h"
#include "state_machine.h"

#define TARGET 500
#define TOL 10
static QcEvent step(QcMachine *s,int32_t g){return QC_Update(s,g,g,true,TARGET,TOL);}
static void feed(QcMachine *s,int32_t g,int n){for(int i=0;i<n;i++)(void)step(s,g);}

/* Full firmware-like pipeline: 20 ms task loop, a new HX711 sample every `period_ms`,
   item placed at 1 s and removed at 5 s. Returns PASS count, fills beeps/last verdict. */
static int pipeline(int period_ms,float item,int noise_g,int *beeps,int *passed_total){
 Scale_t sc;Scale_Init(&sc,0);assert(Scale_Calibrate(&sc,1000000,1000));
 QcMachine m;QC_Init(&m);int next=0,pass=0;*beeps=0;srand(7);
 for(int t=0;t<9000;t+=20){
  if(t>=next){
   next+=period_ms;
   float load=(t>=1000&&t<5000)?item:0.0f;
   if(t>=5000&&t<5400)load=item*(1.0f-(float)(t-5000)/400.0f); /* item is lifted gradually */
   int32_t raw=(int32_t)(load*1000.0f)+(noise_g?(rand()%(2*noise_g*1000+1))-noise_g*1000:0);
   (void)Scale_FilterGrams(&sc,raw);
   QcState before=m.state;
   QcEvent e=QC_Update(&m,Scale_GrossGrams(&sc,raw),Scale_NetGrams(&sc,raw),true,TARGET,TOL);
   if(e.completed&&e.completed_pass)pass++;
   if((m.state==QC_PASS||m.state==QC_FAIL)&&before!=QC_PASS&&before!=QC_FAIL)(*beeps)++;
  }
 }
 *passed_total=pass;return pass;
}

int main(void){
 QcMachine s;QC_Init(&s);QcEvent e;

 /* empty -> IDLE */
 feed(&s,0,6);assert(s.state==QC_IDLE);

 /* 480 g -> FAIL, needs STABLE_SAMPLES samples, removal needs REMOVAL_SAMPLES samples */
 feed(&s,480,STABLE_SAMPLES-1);assert(s.state==QC_MEASURING);
 feed(&s,480,1);assert(s.state==QC_FAIL);
 feed(&s,480,3);assert(s.state==QC_WAIT_REMOVE);
 for(int i=0;i<REMOVAL_SAMPLES-1;i++)assert(!step(&s,0).completed);
 e=step(&s,0);assert(e.completed&&!e.completed_pass&&e.completed_g==480&&s.state==QC_IDLE);

 /* 498 g -> PASS, no double count while it sits there, counted once on removal */
 feed(&s,498,STABLE_SAMPLES);assert(s.state==QC_PASS);
 for(int i=0;i<50;i++)assert(!step(&s,498).completed);
 for(int i=0;i<REMOVAL_SAMPLES-1;i++)assert(!step(&s,0).completed);
 e=step(&s,0);assert(e.completed&&e.completed_pass&&e.completed_g==498);
 assert(!step(&s,0).completed);

 /* a single glitch below PRESENCE while the item stays must NOT complete the cycle */
 feed(&s,500,STABLE_SAMPLES);assert(s.state==QC_PASS);
 assert(!step(&s,3).completed);assert(!step(&s,500).completed);
 assert(!step(&s,3).completed);assert(!step(&s,3).completed);assert(!step(&s,500).completed);
 assert(s.state==QC_WAIT_REMOVE);
 feed(&s,0,REMOVAL_SAMPLES);

 /* adding more material re-arms the measurement; result is the mean of the stable window */
 feed(&s,430,STABLE_SAMPLES);assert(s.state==QC_FAIL);
 /* a single spike / bump must NOT discard the result (debounce) ... */
 (void)step(&s,500);assert(s.state==QC_WAIT_REMOVE&&s.verdict==0);
 (void)step(&s,430);assert(s.state==QC_WAIT_REMOVE&&s.verdict==0);
 for(int i=0;i<REARM_SAMPLES-1;i++)(void)step(&s,500);
 assert(s.state==QC_WAIT_REMOVE&&s.verdict==0);
 /* ... but REARM_SAMPLES consecutive high samples re-arm the measurement */
 (void)step(&s,500);assert(s.state==QC_MEASURING&&s.verdict==-1);
 feed(&s,500,STABLE_SAMPLES-1);assert(s.state==QC_PASS&&s.locked_g==500);
 feed(&s,0,REMOVAL_SAMPLES);

 /* noisy but stable window is averaged: 498,500,502,500 -> 500 */
 (void)step(&s,498);(void)step(&s,500);(void)step(&s,502);(void)step(&s,500);
 assert(s.state==QC_PASS&&s.locked_g==500);
 feed(&s,0,REMOVAL_SAMPLES);

 /* not stable (window range > STABLE_DELTA_G) -> keeps measuring */
 (void)step(&s,300);(void)step(&s,310);(void)step(&s,300);(void)step(&s,310);
 assert(s.state==QC_MEASURING);
 feed(&s,0,REMOVAL_SAMPLES+1);

 /* Regression (PASS but PCS did not increase): lifting the item is gradual, the scale passes
    through intermediate weights. The PASS verdict must survive and be counted. */
 feed(&s,500,STABLE_SAMPLES);assert(s.state==QC_PASS);
 (void)step(&s,380);(void)step(&s,240);(void)step(&s,90);
 assert(s.state==QC_WAIT_REMOVE);
 (void)step(&s,0);(void)step(&s,0);
 e=step(&s,0);assert(e.completed&&e.completed_pass&&e.completed_g==500&&s.state==QC_IDLE);
 /* same for FAIL: counted as completed but not as pass */
 feed(&s,470,STABLE_SAMPLES);assert(s.state==QC_FAIL);
 (void)step(&s,300);(void)step(&s,100);feed(&s,0,REMOVAL_SAMPLES-1);
 e=step(&s,0);assert(e.completed&&!e.completed_pass);
 /* if the lower weight really settles (partial removal), it is measured again */
 feed(&s,500,STABLE_SAMPLES);assert(s.state==QC_PASS);
 feed(&s,300,STABLE_SAMPLES);assert(s.state==QC_FAIL&&s.locked_g==300);
 feed(&s,0,REMOVAL_SAMPLES);

 /* overload / sensor error */
 (void)QC_Update(&s,5000,0,true,TARGET,TOL);assert(s.state==QC_OVERLOAD);
 (void)QC_Update(&s,0,0,false,TARGET,TOL);assert(s.state==QC_ERROR);
 feed(&s,0,2);assert(s.state==QC_IDLE);

 /* Regression for the sample-rate bug: with a 10 SPS HX711 (new sample every 100 ms) and a
    20 ms task loop, a 500 g item must give exactly ONE beep and ONE PASS; 520 g must FAIL. */
 int beeps,passes;
 pipeline(100,500.0f,0,&beeps,&passes);assert(beeps==1&&passes==1);
 pipeline(100,500.0f,2,&beeps,&passes);assert(beeps==1&&passes==1);
 pipeline(100,520.0f,0,&beeps,&passes);assert(beeps==1&&passes==0);
 pipeline(100,505.0f,0,&beeps,&passes);assert(beeps==1&&passes==1);
 pipeline(13,500.0f,2,&beeps,&passes);assert(beeps==1&&passes==1);
 /* rearm counter must not leak across cycles (fault in the middle) */
 feed(&s,0,REMOVAL_SAMPLES+1);
 feed(&s,500,STABLE_SAMPLES);assert(s.state==QC_PASS);
 (void)step(&s,560);(void)step(&s,560);            /* 2 of REARM_SAMPLES high samples */
 (void)QC_Update(&s,0,0,false,TARGET,TOL);assert(s.state==QC_ERROR);
 feed(&s,0,2);feed(&s,500,STABLE_SAMPLES);assert(s.state==QC_PASS);
 (void)step(&s,560);assert(s.state==QC_WAIT_REMOVE&&s.verdict==1);  /* one bump: result kept */
 puts("QC state-machine tests: PASS");return 0;
}
