#ifndef APP_TYPES_H
#define APP_TYPES_H
/*
 * Các kiểu dữ liệu dùng chung giữa các task (giao ước interface của nhóm).
 */
#include <stdbool.h>
#include <stdint.h>

/* Trạng thái máy QC (xem sơ đồ trong README). */
typedef enum {
    QC_INIT = 0,       /* vừa khởi tạo / vừa reset                       */
    QC_IDLE,           /* bàn cân trống                                   */
    QC_MEASURING,      /* có vật, đang chờ số ổn định                     */
    QC_PASS,           /* vừa chốt kết quả: đạt                           */
    QC_FAIL,           /* vừa chốt kết quả: không đạt                     */
    QC_WAIT_REMOVE,    /* đã có kết quả, chờ nhấc vật ra                  */
    QC_OVERLOAD,       /* quá tải                                         */
    QC_ERROR           /* mất tín hiệu HX711                              */
} QcState;

/* Màn hình đang hiển thị (chế độ giao diện). */
typedef enum {
    VIEW_LIVE = 0,     /* màn hình đo chính                               */
    VIEW_SET_TARGET,   /* chỉnh khối lượng chuẩn                          */
    VIEW_SET_TOL,      /* chỉnh dung sai                                  */
    VIEW_CAL_SET_MASS, /* CAL bước 1: chọn khối lượng quả cân mẫu         */
    VIEW_CAL_ZERO,     /* CAL bước 2: lấy điểm 0 (bàn cân trống)          */
    VIEW_CAL_WEIGHT    /* CAL bước 3: đặt quả cân mẫu, tính hệ số         */
} ViewMode;

/* Kiểu tiếng kêu QCTask yêu cầu BuzzerTask phát. */
typedef enum {
    ALARM_SILENT = 0,
    ALARM_PASS,        /* 2 tiếng ngắn                                    */
    ALARM_FAIL,        /* 1 tiếng dài                                     */
    ALARM_OVERLOAD,    /* kêu liên tục nhịp nhanh                         */
    ALARM_ERROR        /* kêu liên tục nhịp chậm                          */
} AlarmMode;

/* HX711Task -> QCTask: một mẫu ADC thô. */
typedef struct {
    int32_t  raw;
    uint32_t timestamp_ms;
} HxSample;

/* QCTask -> OLEDTask: mọi thứ màn hình cần vẽ (một "ảnh chụp" trạng thái). */
typedef struct {
    ViewMode view;
    QcState  state;
    int8_t   verdict;        /* -1 chưa có, 0 FAIL, 1 PASS                   */
    bool     sensor_ok;
    bool     calibrated;
    bool     busy;           /* đang gom mẫu cho TARE / CAL                  */
    bool     cal_failed;     /* lần CAL vừa rồi không hợp lệ                 */

    int32_t  filtered_g;     /* khối lượng đã lọc (để hiển thị)              */
    int32_t  instant_g;      /* khối lượng tức thời (chẩn đoán)              */
    int32_t  locked_g;       /* kết quả đã chốt                              */

    uint32_t target_g;
    uint32_t tolerance_g;
    uint32_t cal_mass_g;
    uint32_t pass_count;
    uint32_t button_mask;    /* nút đang nhấn (chẩn đoán)                    */
} DisplayFrame;

#endif /* APP_TYPES_H */
