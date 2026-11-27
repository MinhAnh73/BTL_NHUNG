#include "scale.h"
#include <math.h>

#define EMA_ALPHA             0.2f     /* 0.2 = lọc mạnh, phản hồi chậm hơn */

static int32_t round_to_int(float x)
{
    return (int32_t)(x >= 0.0f ? x + 0.5f : x - 0.5f);
}

static float raw_to_grams(const Scale_t *s, int32_t raw, int32_t zero_raw)
{
    return ((float)raw - (float)zero_raw) / s->counts_per_gram;
}

static void reset_filter(Scale_t *s)
{
    for (int i = 0; i < MEDIAN_SIZE; i++) {
        s->history[i] = 0.0f;
    }
    s->history_idx   = 0;
    s->filtered_g    = 0.0f;
    s->filter_seeded = false;
}

void Scale_Init(Scale_t *s, int32_t start_raw)
{
    s->start_zero_raw  = start_raw;
    s->tare_raw        = start_raw;
    s->counts_per_gram = DEFAULT_COUNTS_PER_GRAM;
    reset_filter(s);
}

void Scale_Tare(Scale_t *s, int32_t zero_raw)
{
    s->tare_raw = zero_raw;
    reset_filter(s);
}

bool Scale_Calibrate(Scale_t *s, int32_t loaded_raw, uint32_t known_mass_g)
{
    if (known_mass_g < CAL_MIN_G || known_mass_g > CAL_MAX_G) {
        return false;
    }
    float factor = ((float)loaded_raw - (float)s->tare_raw) / (float)known_mass_g;
    if (!isfinite(factor) || fabsf(factor) < MIN_COUNTS_PER_GRAM || fabsf(factor) > MAX_COUNTS_PER_GRAM) {
        return false;   /* chưa đặt quả cân, hoặc loadcell nối sai */
    }
    s->counts_per_gram = factor;
    reset_filter(s);
    return true;
}

int32_t Scale_NetGrams(const Scale_t *s, int32_t raw)
{
    return round_to_int(raw_to_grams(s, raw, s->tare_raw));
}

int32_t Scale_GrossGrams(const Scale_t *s, int32_t raw)
{
    return round_to_int(raw_to_grams(s, raw, s->start_zero_raw));
}

/* Lọc 2 tầng cho số hiển thị mượt:
   1) Trung vị 5 mẫu: loại bỏ gai nhiễu đơn lẻ.
   2) EMA: filtered = 0.2 * mới + 0.8 * cũ. */
int32_t Scale_FilterGrams(Scale_t *s, int32_t raw)
{
    s->history[s->history_idx] = raw_to_grams(s, raw, s->tare_raw);
    s->history_idx = (uint8_t)((s->history_idx + 1u) % MEDIAN_SIZE);

    float sorted[MEDIAN_SIZE];
    for (int i = 0; i < MEDIAN_SIZE; i++) {
        sorted[i] = s->history[i];
    }
    for (int i = 0; i < MEDIAN_SIZE - 1; i++) {          /* sắp xếp chọn, 5 phần tử */
        for (int j = i + 1; j < MEDIAN_SIZE; j++) {
            if (sorted[j] < sorted[i]) {
                float tmp = sorted[i];
                sorted[i] = sorted[j];
                sorted[j] = tmp;
            }
        }
    }
    float median = sorted[MEDIAN_SIZE / 2];

    if (!s->filter_seeded) {
        s->filtered_g    = median;
        s->filter_seeded = true;
    } else {
        s->filtered_g = EMA_ALPHA * median + (1.0f - EMA_ALPHA) * s->filtered_g;
    }
    return round_to_int(s->filtered_g);
}

/* ------------------------------ RawAverager ------------------------------ */

void RawAvg_Reset(RawAverager *a)
{
    a->count = 0;
    a->sum   = 0;
    a->min   = 0;
    a->max   = 0;
}

void RawAvg_Add(RawAverager *a, int32_t raw)
{
    if (a->count == 0 || raw < a->min) a->min = raw;
    if (a->count == 0 || raw > a->max) a->max = raw;
    a->sum += raw;
    if (a->count < 255u) {
        a->count++;
    }
}

bool RawAvg_IsFull(const RawAverager *a)
{
    return a->count >= RAW_AVG_SAMPLES;
}

int32_t RawAvg_Mean(const RawAverager *a)
{
    if (a->count == 0) {
        return 0;
    }
    return (int32_t)(a->sum / a->count);
}

int32_t RawAvg_Spread(const RawAverager *a)
{
    return a->max - a->min;
}
