#ifndef APP_TYPES_H
#define APP_TYPES_H
#include <stdbool.h>
#include <stdint.h>
typedef enum {QC_INIT=0, QC_IDLE, QC_MEASURING, QC_STABLE, QC_PASS, QC_FAIL,
              QC_WAIT_REMOVE, QC_OVERLOAD, QC_ERROR} QcState;
typedef enum {VIEW_LIVE=0,VIEW_SET_TARGET,VIEW_SET_TOL,VIEW_CAL_SET_MASS,
              VIEW_CAL_ZERO,VIEW_CAL_WEIGHT} ViewMode;
typedef enum {ALARM_SILENT=0, ALARM_PASS, ALARM_FAIL, ALARM_OVERLOAD, ALARM_ERROR} AlarmMode;
/* All values below are held in RAM, reset on power loss. */
typedef struct {uint32_t pass_count,target_g,tolerance_g;uint8_t calibrated;} QcSettings;
typedef struct {int32_t raw;uint32_t timestamp_ms;} HxSample;
typedef struct {int32_t grams;uint32_t pass_count,target_g,tolerance_g,cal_mass_g;
                uint8_t calibrated,sensor_ok;QcState state;ViewMode view;int8_t verdict;} DisplayFrame;
#endif
