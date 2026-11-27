#include "state_machine.h"

/* ============================ Cửa sổ mẫu ============================ */

static void window_clear(QcMachine *m)
{
    m->window_count = 0;
    m->window_next  = 0;
}

static void window_push(QcMachine *m, int32_t g)
{
    m->window[m->window_next] = g;
    m->window_next = (uint8_t)((m->window_next + 1u) % STABLE_SAMPLES);
    if (m->window_count < STABLE_SAMPLES) {
        m->window_count++;
    }
}

/* Cửa sổ đã đầy và (max - min) <= STABLE_DELTA_G ? */
static bool window_is_stable(const QcMachine *m)
{
    if (m->window_count < STABLE_SAMPLES) {
        return false;
    }
    int32_t lo = m->window[0];
    int32_t hi = m->window[0];
    for (unsigned i = 1; i < STABLE_SAMPLES; i++) {
        if (m->window[i] < lo) lo = m->window[i];
        if (m->window[i] > hi) hi = m->window[i];
    }
    return (hi - lo) <= STABLE_DELTA_G;
}

/* Trung bình cửa sổ, làm tròn đến gam gần nhất. */
static int32_t window_mean(const QcMachine *m)
{
    int64_t sum = 0;
    for (unsigned i = 0; i < STABLE_SAMPLES; i++) {
        sum += m->window[i];
    }
    int64_t half = STABLE_SAMPLES / 2;
    return (int32_t)((sum >= 0 ? sum + half : sum - half) / STABLE_SAMPLES);
}

/* ============================ Chuyển trạng thái ============================ */

static void go_idle(QcMachine *m)
{
    m->state        = QC_IDLE;
    m->verdict      = -1;
    m->absent_count = 0;
    m->rearm_count  = 0;
    window_clear(m);
}

static void go_fault(QcMachine *m, QcState fault)
{
    m->state        = fault;
    m->absent_count = 0;
    m->rearm_count  = 0;
    window_clear(m);
}

static void start_new_measurement(QcMachine *m)
{
    m->state   = QC_MEASURING;
    m->verdict = -1;
}

static bool in_tolerance(int32_t g, uint32_t target_g, uint32_t tolerance_g)
{
    int64_t low  = (int64_t)target_g - (int64_t)tolerance_g;
    int64_t high = (int64_t)target_g + (int64_t)tolerance_g;
    return (int64_t)g >= low && (int64_t)g <= high;
}

/* ============================ Giai đoạn 1: đã có kết quả ============================ */

typedef enum {
    AFTER_RESULT_STAY,          /* giữ kết quả, chờ tiếp                          */
    AFTER_RESULT_MEASURE,       /* hủy kết quả, đo lại từ đầu (cửa sổ rỗng)       */
    AFTER_RESULT_MEASURE_READY  /* hủy kết quả, cửa sổ đã có sẵn mức mới ổn định  */
} AfterResult;

/* Xử lý khi đang PASS / FAIL / WAIT_REMOVE. Có 4 khả năng:
   a) Bàn cân trống đủ REMOVAL_SAMPLES mẫu  -> hoàn tất sản phẩm (ev->completed).
   b) Vẫn là vật cũ (lệch <= REARM_DELTA_G) -> giữ nguyên.
   c) Nặng hơn hẳn trong REARM_SAMPLES mẫu  -> thêm vật, đo lại.
   d) Nhẹ hơn: thường là đang nhấc vật ra (số giảm dần) -> giữ kết quả;
      chỉ đo lại nếu mức thấp hơn đứng yên thật (bỏ bớt một phần vật). */
static AfterResult handle_result(QcMachine *m, int32_t net_g, QcEvent *ev)
{
    /* a) không có vật */
    if (net_g < PRESENCE_G) {
        m->state = QC_WAIT_REMOVE;
        m->absent_count++;
        if (m->absent_count >= REMOVAL_SAMPLES) {
            ev->completed      = true;
            ev->completed_pass = (m->verdict == 1);
            ev->completed_g    = m->locked_g;
            go_idle(m);
        }
        return AFTER_RESULT_STAY;
    }
    m->absent_count = 0;              /* nhấc ra phải LIÊN TIẾP, một gai nhiễu không tính */

    /* b) vẫn là vật cũ */
    int32_t diff = net_g - m->locked_g;
    if (diff >= -REARM_DELTA_G && diff <= REARM_DELTA_G) {
        m->state       = QC_WAIT_REMOVE;
        m->rearm_count = 0;
        window_clear(m);
        return AFTER_RESULT_STAY;
    }

    /* c) nặng hơn: thêm vật */
    if (diff > 0) {
        m->state = QC_WAIT_REMOVE;
        m->rearm_count++;
        if (m->rearm_count < REARM_SAMPLES) {
            return AFTER_RESULT_STAY;     /* có thể chỉ là va chạm, chờ thêm */
        }
        m->rearm_count = 0;
        window_clear(m);
        start_new_measurement(m);
        return AFTER_RESULT_MEASURE;
    }

    /* d) nhẹ hơn */
    m->rearm_count = 0;
    window_push(m, net_g);
    if (!window_is_stable(m)) {
        m->state = QC_WAIT_REMOVE;
        return AFTER_RESULT_STAY;
    }
    start_new_measurement(m);
    return AFTER_RESULT_MEASURE_READY;
}

/* ============================ Giai đoạn 2: đo ============================ */

static void measure(QcMachine *m, int32_t net_g, bool window_ready,
                    uint32_t target_g, uint32_t tolerance_g)
{
    if (net_g < PRESENCE_G) {
        go_idle(m);                        /* vật bị nhấc ra trước khi có kết quả */
        return;
    }
    if (m->state == QC_IDLE) {
        window_clear(m);
        start_new_measurement(m);
    }
    if (!window_ready) {
        window_push(m, net_g);
    }
    if (!window_is_stable(m)) {
        m->state = QC_MEASURING;
        return;
    }

    /* Chốt kết quả */
    m->locked_g    = window_mean(m);
    m->verdict     = in_tolerance(m->locked_g, target_g, tolerance_g) ? 1 : 0;
    m->state       = (m->verdict == 1) ? QC_PASS : QC_FAIL;
    m->rearm_count = 0;
}

/* ============================ API ============================ */

void QC_Init(QcMachine *m)
{
    go_idle(m);
    m->state    = QC_INIT;
    m->locked_g = 0;
}

QcEvent QC_Update(QcMachine *m, int32_t gross_g, int32_t net_g, bool sensor_ok,
                  uint32_t target_g, uint32_t tolerance_g)
{
    QcEvent ev = { false, false, 0 };

    /* Lỗi có ưu tiên cao nhất */
    if (!sensor_ok) {
        go_fault(m, QC_ERROR);
        return ev;
    }
    if (gross_g > PHYSICAL_OVERLOAD_G || gross_g < -PHYSICAL_OVERLOAD_G) {
        go_fault(m, QC_OVERLOAD);
        return ev;
    }
    if (m->state == QC_ERROR || m->state == QC_OVERLOAD || m->state == QC_INIT) {
        go_idle(m);                        /* hết lỗi -> bắt đầu lại từ IDLE */
    }

    bool window_ready = false;
    if (QC_HasResult(m->state)) {
        AfterResult r = handle_result(m, net_g, &ev);
        if (r == AFTER_RESULT_STAY) {
            return ev;
        }
        window_ready = (r == AFTER_RESULT_MEASURE_READY);
    }

    measure(m, net_g, window_ready, target_g, tolerance_g);
    return ev;
}

bool QC_HasResult(QcState state)
{
    return state == QC_PASS || state == QC_FAIL || state == QC_WAIT_REMOVE;
}

const char *QC_StateName(QcState state)
{
    switch (state) {
    case QC_INIT:        return "INIT";
    case QC_IDLE:        return "IDLE";
    case QC_MEASURING:   return "MEASURING";
    case QC_PASS:        return "PASS";
    case QC_FAIL:        return "FAIL";
    case QC_WAIT_REMOVE: return "REMOVE ITEM";
    case QC_OVERLOAD:    return "OVERLOAD";
    case QC_ERROR:       return "SENSOR ERROR";
    default:             return "UNKNOWN";
    }
}
