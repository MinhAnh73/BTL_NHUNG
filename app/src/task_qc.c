/*
 * QCTask — "bộ não" của cân. Là task DUY NHẤT được thay đổi trạng thái hệ thống:
 *   - nhận mẫu RAW từ HX711Task, đổi sang gam (module scale)
 *   - đọc nút nhấn, điều khiển giao diện (LIVE / SET TARGET / SET TOL / CAL)
 *   - TARE và hiệu chuẩn (CAL)
 *   - chạy máy trạng thái QC (module state_machine), đếm PCS PASS
 *   - gửi DisplayFrame cho OLEDTask, gửi AlarmMode cho BuzzerTask
 *   - giám sát các task khác và "cho ăn" watchdog
 * Vì chỉ một task ghi dữ liệu nên không cần mutex.
 */
#include "task_qc.h"
#include "app_config.h"
#include "app_ipc.h"
#include "button.h"
#include "scale.h"
#include "state_machine.h"
#include <math.h>
#include <string.h>

/* Việc đang gom mẫu (sau khi nhấn TARE hoặc CAL). */
typedef enum {
    CAPTURE_NONE = 0,
    CAPTURE_TARE,          /* TARE ở màn hình LIVE             */
    CAPTURE_CAL_ZERO,      /* CAL bước 2: điểm 0               */
    CAPTURE_CAL_WEIGHT     /* CAL bước 3: quả cân mẫu          */
} CaptureKind;

/* Toàn bộ trạng thái của QCTask gom vào một struct.
   Khai báo static (không nằm trên stack) => xem được trong Keil Watch: "qc". */
typedef struct {
    /* đo lường */
    Scale_t     scale;
    QcMachine   fsm;
    bool        have_sample;       /* đã nhận ít nhất 1 mẫu từ HX711          */
    uint32_t    last_sample_ms;
    int32_t     raw;
    int32_t     net_g;             /* tức thời, đã trừ bì (dùng cho QC)       */
    int32_t     gross_g;           /* so với lúc bật nguồn (quá tải)          */
    int32_t     filtered_g;        /* đã lọc (hiển thị)                       */

    /* cài đặt (chỉ trong RAM, mất khi tắt nguồn) */
    bool        calibrated;
    uint32_t    target_g;
    uint32_t    tolerance_g;
    uint32_t    cal_mass_g;
    uint32_t    pass_count;

    /* giao diện */
    ViewMode    view;
    CaptureKind capture;
    RawAverager capture_avg;
    bool        cal_failed;
    bool        wait_empty_after_cal;   /* sau CAL phải nhấc quả cân ra rồi mới đo */

    AlarmMode   active_fault;           /* OVERLOAD / ERROR đang báo               */
} QcContext;

static QcContext qc;

/* ============================== Tiện ích ============================== */

/* value + step nếu kết quả nằm trong [min, max], ngược lại giữ nguyên value. */
static int32_t step_if_in_range(int32_t value, int32_t step, int32_t min, int32_t max)
{
    int32_t next = value + step;
    return (next < min || next > max) ? value : next;
}

static bool in_cal_view(void)
{
    return qc.view == VIEW_CAL_SET_MASS || qc.view == VIEW_CAL_ZERO || qc.view == VIEW_CAL_WEIGHT;
}

static void reset_qc_cycle(void)
{
    QC_Init(&qc.fsm);
}

/* ============================== Mẫu HX711 ============================== */

static void on_new_sample(const HxSample *s)
{
    qc.raw            = s->raw;
    qc.last_sample_ms = s->timestamp_ms;

    if (!qc.have_sample) {
        Scale_Init(&qc.scale, s->raw);    /* mẫu đầu tiên = điểm 0 tạm thời */
        qc.have_sample = true;
    }
    qc.filtered_g = Scale_FilterGrams(&qc.scale, s->raw);
    qc.net_g      = Scale_NetGrams(&qc.scale, s->raw);
    qc.gross_g    = Scale_GrossGrams(&qc.scale, s->raw);
}

/* Cảm biến OK nếu mẫu cuối cùng không quá SENSOR_TIMEOUT_MS.
   Ép sang int32 vì mẫu có thể "mới hơn" now_ms một chút (HX711Task chạy xen vào). */
static bool sensor_is_ok(uint32_t now_ms)
{
    return qc.have_sample && (int32_t)(now_ms - qc.last_sample_ms) <= (int32_t)SENSOR_TIMEOUT_MS;
}

/* ============================== TARE / CAL ============================== */

static void start_capture(CaptureKind kind)
{
    qc.capture    = kind;
    qc.cal_failed = false;
    RawAvg_Reset(&qc.capture_avg);
    reset_qc_cycle();
}

static void cancel_capture(void)
{
    qc.capture = CAPTURE_NONE;
}

/* Độ dao động của các mẫu vừa gom, đổi ra gam.
   Khi CAL quả cân, hệ số cũ chưa đúng => ước lượng hệ số từ chính lần đo này. */
static float capture_spread_g(void)
{
    float counts_per_gram = qc.scale.counts_per_gram;

    if (qc.capture == CAPTURE_CAL_WEIGHT) {
        float delta = (float)RawAvg_Mean(&qc.capture_avg) - (float)qc.scale.tare_raw;
        counts_per_gram = delta / (float)qc.cal_mass_g;
    }
    if (fabsf(counts_per_gram) < MIN_COUNTS_PER_GRAM) {
        return 0.0f;      /* chưa đặt quả cân: để Scale_Calibrate báo lỗi */
    }
    return (float)RawAvg_Spread(&qc.capture_avg) / fabsf(counts_per_gram);
}

static void finish_capture(int32_t mean_raw)
{
    switch (qc.capture) {
    case CAPTURE_TARE:
        Scale_Tare(&qc.scale, mean_raw);
        break;

    case CAPTURE_CAL_ZERO:
        Scale_Tare(&qc.scale, mean_raw);
        qc.view = VIEW_CAL_WEIGHT;
        break;

    case CAPTURE_CAL_WEIGHT:
        if (Scale_Calibrate(&qc.scale, mean_raw, qc.cal_mass_g)) {
            qc.calibrated           = true;
            qc.wait_empty_after_cal = true;
            qc.view                 = VIEW_LIVE;
            AppIPC_Beep(ALARM_PASS);
        } else {
            qc.cal_failed = true;
            AppIPC_Beep(ALARM_FAIL);
        }
        break;

    default:
        break;
    }
    qc.capture = CAPTURE_NONE;
    reset_qc_cycle();
}

/* Gọi với mỗi mẫu mới khi đang gom. Đủ RAW_AVG_SAMPLES mẫu:
   - đứng yên (dao động <= CAPTURE_MAX_SPREAD_G) => áp dụng trung bình
   - còn rung => bỏ, gom lại từ đầu */
static void update_capture(void)
{
    RawAvg_Add(&qc.capture_avg, qc.raw);
    if (!RawAvg_IsFull(&qc.capture_avg)) {
        return;
    }
    if (capture_spread_g() <= (float)CAPTURE_MAX_SPREAD_G) {
        finish_capture(RawAvg_Mean(&qc.capture_avg));
    } else {
        RawAvg_Reset(&qc.capture_avg);
    }
}

/* ============================== Nút nhấn ============================== */

static void handle_mode_button(void)
{
    cancel_capture();
    if (in_cal_view()) {
        qc.view = VIEW_LIVE;                       /* hủy hiệu chuẩn */
    } else if (qc.view == VIEW_LIVE) {
        qc.view = VIEW_SET_TARGET;
    } else if (qc.view == VIEW_SET_TARGET) {
        qc.view = VIEW_SET_TOL;
    } else {
        qc.view = VIEW_LIVE;
    }
    reset_qc_cycle();
}

static void handle_up_down(int32_t direction)      /* +1 = UP, -1 = DOWN */
{
    if (qc.view == VIEW_SET_TARGET) {
        qc.target_g = (uint32_t)step_if_in_range((int32_t)qc.target_g, direction * TARGET_STEP_G,
                                           TARGET_MIN_G, TARGET_MAX_G);
        reset_qc_cycle();
    } else if (qc.view == VIEW_SET_TOL) {
        qc.tolerance_g = (uint32_t)step_if_in_range((int32_t)qc.tolerance_g, direction * TOLERANCE_STEP_G,
                                              TOLERANCE_MIN_G, TOLERANCE_MAX_G);
        reset_qc_cycle();
    } else if (qc.view == VIEW_CAL_SET_MASS) {
        qc.cal_mass_g = (uint32_t)step_if_in_range((int32_t)qc.cal_mass_g, direction * (int32_t)CAL_STEP_G,
                                             (int32_t)CAL_MIN_G, (int32_t)CAL_MAX_G);
    }
}

static void handle_buttons(uint32_t keys, bool sensor_ok)
{
    /* Các nút luôn hoạt động (kể cả khi đang gom mẫu) */
    if (keys & BUTTON_MODE) {
        handle_mode_button();
        return;
    }
    if (keys & BUTTON_CAL_LONG) {                  /* giữ CAL: vào hiệu chuẩn */
        cancel_capture();
        qc.view       = VIEW_CAL_SET_MASS;
        qc.cal_failed = false;
        reset_qc_cycle();
        return;
    }
    if (qc.capture != CAPTURE_NONE) {
        return;                                    /* đang gom mẫu: bỏ qua nút khác */
    }

    if (keys & BUTTON_UP)   handle_up_down(+1);
    if (keys & BUTTON_DOWN) handle_up_down(-1);

    if (keys & BUTTON_CAL_SHORT) {
        if (qc.view == VIEW_CAL_SET_MASS) {
            qc.view = VIEW_CAL_ZERO;
        } else if (qc.view == VIEW_CAL_WEIGHT && sensor_ok) {
            start_capture(CAPTURE_CAL_WEIGHT);
        }
    }

    if ((keys & BUTTON_TARE) && sensor_ok) {
        if (qc.view == VIEW_CAL_ZERO) {
            start_capture(CAPTURE_CAL_ZERO);
        } else if (qc.view == VIEW_LIVE) {
            qc.wait_empty_after_cal = false;
            start_capture(CAPTURE_TARE);
        }
    }

    if ((keys & BUTTON_RESET_LONG) && qc.view == VIEW_LIVE && qc.fsm.state == QC_IDLE) {
        qc.pass_count = 0;
    }
}

/* ============================== QC ============================== */

static void run_qc(bool fresh, bool sensor_ok)
{
    if (!sensor_ok) {
        if (qc.have_sample) {
            (void)QC_Update(&qc.fsm, qc.gross_g, qc.net_g, false, qc.target_g, qc.tolerance_g);
        }
        cancel_capture();
        return;
    }

    /* Chỉ đo QC ở màn hình LIVE, đã hiệu chuẩn và không bận TARE. */
    if (qc.view != VIEW_LIVE || !qc.calibrated || qc.capture != CAPTURE_NONE) {
        reset_qc_cycle();
        return;
    }

    /* Ngay sau CAL quả cân còn trên bàn: chờ nhấc ra, không tính là sản phẩm. */
    if (qc.wait_empty_after_cal) {
        reset_qc_cycle();
        if (qc.net_g < PRESENCE_G && qc.net_g > -PRESENCE_G) {
            qc.wait_empty_after_cal = false;
        }
        return;
    }

    if (!fresh) {
        return;            /* QC đếm theo MẪU MỚI, không theo vòng lặp */
    }

    QcState before = qc.fsm.state;
    QcEvent ev = QC_Update(&qc.fsm, qc.gross_g, qc.net_g, true, qc.target_g, qc.tolerance_g);

    if (ev.completed && ev.completed_pass && qc.pass_count < 0xFFFFFFFFu) {
        qc.pass_count++;                           /* PCS tăng khi nhấc vật PASS ra */
    }

    bool just_decided = (qc.fsm.state == QC_PASS || qc.fsm.state == QC_FAIL) &&
                        !(before == QC_PASS || before == QC_FAIL);
    if (just_decided) {
        AppIPC_Beep(qc.fsm.state == QC_PASS ? ALARM_PASS : ALARM_FAIL);
    }
}

/* Báo động liên tục khi OVERLOAD / ERROR; tắt khi hết lỗi. */
static void update_fault_alarm(void)
{
    AlarmMode fault = ALARM_SILENT;
    if (qc.fsm.state == QC_OVERLOAD) fault = ALARM_OVERLOAD;
    if (qc.fsm.state == QC_ERROR)    fault = ALARM_ERROR;

    if (fault != qc.active_fault) {
        AppIPC_Beep(fault);
        qc.active_fault = fault;
    }
}

/* ============================== Hiển thị / watchdog ============================== */

static void send_display_frame(bool sensor_ok)
{
    DisplayFrame f;
    memset(&f, 0, sizeof f);

    f.view        = qc.view;
    f.state       = qc.fsm.state;
    f.verdict     = qc.fsm.verdict;
    f.sensor_ok   = sensor_ok;
    f.calibrated  = qc.calibrated;
    f.busy        = (qc.capture != CAPTURE_NONE);
    f.cal_failed  = qc.cal_failed;
    f.filtered_g  = qc.filtered_g;
    f.instant_g   = qc.net_g;
    f.locked_g    = qc.fsm.locked_g;
    f.target_g    = qc.target_g;
    f.tolerance_g = qc.tolerance_g;
    f.cal_mass_g  = qc.cal_mass_g;
    f.pass_count  = qc.pass_count;
    f.button_mask = Button_RawMask();

    AppIPC_SendDisplay(&f);
}

/* Mỗi HEALTH_CHECK_MS: kiểm tra 4 task còn chạy. Chỉ "cho ăn" IWDG khi tất cả OK,
   nên nếu một task bị treo, IWDG sẽ reset MCU sau ~2 s. */
static void service_health(uint32_t now_ms)
{
    static uint32_t last_check_ms = 0;
    static bool     healthy       = true;

    if ((uint32_t)(now_ms - last_check_ms) >= HEALTH_CHECK_MS) {
        last_check_ms = now_ms;
        healthy = AppIPC_CheckHealth();
    }
#if ENABLE_IWDG
    if (healthy) {
        IWDG->KR = 0xAAAAu;
    }
#else
    (void)healthy;
#endif
}

/* ============================== Task ============================== */

static void QC_Task(void *arg)
{
    (void)arg;

    memset(&qc, 0, sizeof qc);
    qc.target_g    = DEFAULT_TARGET_G;
    qc.tolerance_g = DEFAULT_TOLERANCE_G;
    qc.cal_mass_g  = CAL_DEFAULT_G;
    qc.view        = VIEW_LIVE;
    QC_Init(&qc.fsm);
    Button_Init();

    TickType_t wake = xTaskGetTickCount();

    for (;;) {
        uint32_t now_ms = HAL_GetTick();

        /* 1. Lấy mẫu mới (nếu có) */
        HxSample sample;
        bool fresh = (xQueueReceive(q_hx_samples, &sample, 0) == pdTRUE);
        if (fresh) {
            on_new_sample(&sample);
        }
        bool sensor_ok = sensor_is_ok(now_ms);

        /* 2. Nút nhấn */
        handle_buttons(Button_Poll(now_ms), sensor_ok);

        /* 3. TARE / CAL đang gom mẫu */
        if (fresh && qc.capture != CAPTURE_NONE) {
            update_capture();
        }

        /* 4. QC + cảnh báo */
        run_qc(fresh, sensor_ok);
        update_fault_alarm();

        /* 5. Gửi màn hình, giám sát */
        send_display_frame(sensor_ok);
        AppIPC_TaskAlive(TASK_ID_QC);
        service_health(now_ms);

        vTaskDelayUntil(&wake, pdMS_TO_TICKS(QC_TASK_PERIOD_MS));
    }
}

BaseType_t Task_QC_Create(void)
{
    return xTaskCreate(QC_Task, "QC", STACK_QC_WORDS, NULL,
                       PRIO_QC, &g_task_handles[TASK_ID_QC]);
}
