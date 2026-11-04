#ifndef APP_TYPES_H
#define APP_TYPES_H
#include <stdbool.h>
#include <stdint.h>
typedef enum {QC_INIT=0, QC_IDLE, QC_MEASURING, QC_STABLE, QC_PASS, QC_FAIL, QC_WAIT_REMOVE, QC_OVERLOAD, QC_ERROR} QcState;
typedef enum {VIEW_LIVE=0,VIEW_SET_TARGET,VIEW_SET_TOL,VIEW_CAL_SET_MASS,VIEW_CAL_ZERO,VIEW_CAL_WEIGHT,VIEW_COUNT} ViewMode;
typedef enum {ALARM_SILENT=0, ALARM_PASS, ALARM_FAIL, ALARM_OVERLOAD, ALARM_ERROR} AlarmMode;
typedef struct {uint32_t pass_count,target_g,tolerance_g;float counts_per_gram;uint8_t calibrated;} PersistData;
typedef struct {int32_t grams;uint32_t pass_count,target_g,tolerance_g;uint8_t storage_ok,calibrated;uint32_t cal_mass_g;QcState state;ViewMode view;int8_t verdict;} DisplayFrame;
typedef struct {bool storage_ok,has_saved_data;PersistData saved;} StorageBoot;
#endif
