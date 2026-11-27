#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H
/*
 * Máy trạng thái QC: quyết định PASS / FAIL cho từng sản phẩm.
 *
 *   IDLE --(có vật)--> MEASURING --(STABLE_SAMPLES mẫu ổn định)--> PASS / FAIL
 *     ^                                                               |
 *     |                                                               v
 *     +----------(nhấc vật ra đủ REMOVAL_SAMPLES mẫu)------------ WAIT_REMOVE
 *
 *   Bất kỳ trạng thái nào: mất cảm biến -> ERROR, quá tải -> OVERLOAD.
 *
 * QUAN TRỌNG: gọi QC_Update() đúng 1 lần cho mỗi mẫu HX711 MỚI (không phải mỗi vòng
 * lặp task), vì mọi bộ đếm ở đây đếm số mẫu thật.
 * Module thuần C => test được trên PC (tests/test_qc_logic.c).
 */
#include "app_types.h"
#include "app_params.h"

typedef struct {
    QcState state;
    int8_t  verdict;                 /* -1 chưa có, 0 FAIL, 1 PASS                       */
    int32_t locked_g;                /* kết quả đã chốt = trung bình cửa sổ ổn định       */

    int32_t window[STABLE_SAMPLES];  /* các mẫu gần nhất (vòng tròn)                      */
    uint8_t window_count;
    uint8_t window_next;

    uint8_t absent_count;            /* số mẫu liên tiếp "không có vật" (khi chờ nhấc ra) */
    uint8_t rearm_count;             /* số mẫu liên tiếp "nặng hơn kết quả" (thêm vật)    */
} QcMachine;

/* Sự kiện trả về: completed = vừa xong 1 sản phẩm (đã nhấc ra). */
typedef struct {
    bool    completed;
    bool    completed_pass;
    int32_t completed_g;
} QcEvent;

void QC_Init(QcMachine *m);

/* gross_g : khối lượng so với lúc bật nguồn (để phát hiện quá tải)
   net_g   : khối lượng đã trừ bì (để đo)                                */
QcEvent QC_Update(QcMachine *m, int32_t gross_g, int32_t net_g, bool sensor_ok,
                  uint32_t target_g, uint32_t tolerance_g);

bool QC_HasResult(QcState state);    /* PASS, FAIL hoặc WAIT_REMOVE */
const char *QC_StateName(QcState state);

#endif /* STATE_MACHINE_H */
