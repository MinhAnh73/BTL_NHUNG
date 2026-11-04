#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H
#include "app_types.h"
typedef struct {
 QcState state;
 int32_t previous_g;
 int32_t locked_g;
 uint8_t stable_samples;
 uint8_t absent_samples;
 int8_t verdict;
} QcMachine;
typedef struct {
 bool completed;
 bool completed_pass;
 int32_t completed_g;
} QcEvent;
void QC_Init(QcMachine *m);
QcEvent QC_Update(QcMachine *m,int32_t grams,int32_t gross_grams,
                  bool sensor_ok,uint32_t target_g,uint32_t tolerance_g);
const char *QC_StateName(QcState state);
#endif
