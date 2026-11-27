/* Test module scale (đổi raw -> gam, TARE, CAL, lọc) và RawAverager trên PC. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "scale.h"

static void test_convert_tare_calibrate(void)
{
    Scale_t s;
    Scale_Init(&s, 1000000);
    assert(Scale_GrossGrams(&s, 1000000) == 0);
    assert(Scale_FilterGrams(&s, 1000000) == 0);

    /* CAL: quả cân 1000 g làm raw tăng 500000 => 500 count/g */
    assert(Scale_Calibrate(&s, 1500000, 1000));
    assert(Scale_GrossGrams(&s, 1500000) == 1000);
    assert(Scale_NetGrams(&s, 1500000) == 1000);

    /* TARE dời điểm 0 của net, nhưng không đổi gross (dùng cho quá tải) */
    Scale_Tare(&s, 1500000);
    assert(Scale_NetGrams(&s, 1500000) == 0);
    assert(Scale_GrossGrams(&s, 1500000) == 1000);
    assert(Scale_NetGrams(&s, 1600000) == 200);
}

static void test_filter(void)
{
    Scale_t s;
    Scale_Init(&s, 0);
    assert(Scale_Calibrate(&s, 500000, 1000));          /* 500 count/g */

    /* hội tụ về giá trị thật */
    for (int i = 0; i < 60; i++) (void)Scale_FilterGrams(&s, 100000);
    assert(Scale_FilterGrams(&s, 100000) == 200);

    /* một gai nhiễu +1000 g bị lọc trung vị bỏ qua */
    int32_t before = Scale_FilterGrams(&s, 100000);
    int32_t spike  = Scale_FilterGrams(&s, 100000 + 500000);
    assert(abs(spike - before) <= 1);
}

static void test_invalid_calibration(void)
{
    Scale_t s;
    Scale_Init(&s, 1000000);
    assert(!Scale_Calibrate(&s, 1600000, 0));
    assert(!Scale_Calibrate(&s, 1600000, CAL_MIN_G - 1));
    assert(!Scale_Calibrate(&s, 1600000, CAL_MAX_G + 1));
    assert(!Scale_Calibrate(&s, 1000001, 1000));        /* chưa đặt quả cân */
    assert(Scale_Calibrate(&s, 1500000, 2000));
    assert(Scale_GrossGrams(&s, 1500000) == 2000);
}

static void test_raw_averager(void)
{
    RawAverager a;
    RawAvg_Reset(&a);
    assert(!RawAvg_IsFull(&a));
    assert(RawAvg_Mean(&a) == 0);

    /* âm và dương, tính đúng trung bình và biên độ */
    const int32_t v[RAW_AVG_SAMPLES] = { -100, -104, -96, -100, -102, -98, -100, -100 };
    for (int i = 0; i < RAW_AVG_SAMPLES; i++) {
        assert(!RawAvg_IsFull(&a));
        RawAvg_Add(&a, v[i]);
    }
    assert(RawAvg_IsFull(&a));
    assert(RawAvg_Mean(&a) == -100);
    assert(RawAvg_Spread(&a) == 8);

    RawAvg_Reset(&a);
    RawAvg_Add(&a, 8000000);
    assert(RawAvg_Mean(&a) == 8000000 && RawAvg_Spread(&a) == 0);
}

int main(void)
{
    test_convert_tare_calibrate();
    test_filter();
    test_invalid_calibration();
    test_raw_averager();
    puts("Scale + RawAverager tests: PASS");
    return 0;
}
