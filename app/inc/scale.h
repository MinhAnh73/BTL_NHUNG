#ifndef SCALE_H
#define SCALE_H
/*
 * Chuyển đổi số ADC thô (raw) của HX711 sang gam.
 *
 *     gam = (raw - tare_raw) / counts_per_gram
 *
 *   - tare_raw        : số raw khi bàn cân trống (điểm 0), đặt bằng TARE.
 *   - counts_per_gram : hệ số, tính bằng hiệu chuẩn với quả cân đã biết khối lượng.
 *   - start_zero_raw  : số raw lúc bật nguồn, dùng để phát hiện quá tải (không bị TARE thay đổi).
 *
 * Module thuần C (không HAL/FreeRTOS) => test được trên PC.
 */
#include <stdbool.h>
#include <stdint.h>
#include "app_params.h"

#define MEDIAN_SIZE 5   /* bộ lọc trung vị 5 mẫu (chỉ dùng cho hiển thị) */

/* Giới hạn hợp lệ của hệ số (count/gam). HX711 + loadcell 5 kg thường ~ 100..2000. */
#define MIN_COUNTS_PER_GRAM   2.0f
#define MAX_COUNTS_PER_GRAM   500000.0f

typedef struct {
    int32_t start_zero_raw;
    int32_t tare_raw;
    float   counts_per_gram;

    /* Bộ lọc hiển thị: trung vị 5 mẫu rồi EMA */
    float   history[MEDIAN_SIZE];
    uint8_t history_idx;
    float   filtered_g;
    bool    filter_seeded;
} Scale_t;

void    Scale_Init(Scale_t *s, int32_t start_raw);
void    Scale_Tare(Scale_t *s, int32_t zero_raw);
/* Tính hệ số từ raw đo được khi đặt quả cân known_mass_g. Trả về false nếu không hợp lệ. */
bool    Scale_Calibrate(Scale_t *s, int32_t loaded_raw, uint32_t known_mass_g);

int32_t Scale_NetGrams(const Scale_t *s, int32_t raw);    /* đã trừ bì (dùng cho QC)       */
int32_t Scale_GrossGrams(const Scale_t *s, int32_t raw);  /* so với lúc bật nguồn (quá tải) */
int32_t Scale_FilterGrams(Scale_t *s, int32_t raw);       /* đã lọc (chỉ để hiển thị)       */

/* ---------------------------------------------------------------------------
 * RawAverager: gom N mẫu raw MỚI để TARE / CAL chính xác hơn 1 mẫu đơn lẻ,
 * đồng thời đo độ dao động (max - min) để biết cân đã đứng yên chưa.
 * ------------------------------------------------------------------------- */
typedef struct {
    uint8_t count;
    int64_t sum;
    int32_t min;
    int32_t max;
} RawAverager;

void    RawAvg_Reset(RawAverager *a);
void    RawAvg_Add(RawAverager *a, int32_t raw);
bool    RawAvg_IsFull(const RawAverager *a);      /* đã đủ RAW_AVG_SAMPLES mẫu */
int32_t RawAvg_Mean(const RawAverager *a);
int32_t RawAvg_Spread(const RawAverager *a);      /* max - min                 */

#endif /* SCALE_H */
