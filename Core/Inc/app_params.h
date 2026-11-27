#ifndef APP_PARAMS_H
#define APP_PARAMS_H
/*
 * Tham số logic của ứng dụng (KHÔNG phụ thuộc phần cứng).
 * File này được dùng chung cho firmware và cho test chạy trên PC (tests/),
 * nên chỉ chứa #define thuần C, không include HAL hay FreeRTOS.
 *
 * Đơn vị: *_G = gam, *_MS = mili giây, *_SAMPLES = số mẫu HX711 MỚI.
 */

/* ---------------- Cân và an toàn ---------------- */
#define SCALE_CAPACITY_G        5000    /* tải trọng loadcell                         */
#define PHYSICAL_OVERLOAD_G     4900    /* vượt mức này (so với lúc bật nguồn) => OVERLOAD */
#define PRESENCE_G              20      /* >= mức này coi là "có vật trên bàn cân"      */

/* ---------------- Máy trạng thái QC ---------------- */
/* "Ổn định" = STABLE_SAMPLES mẫu mới liên tiếp có (max - min) <= STABLE_DELTA_G.
   Kết quả đo = trung bình của cửa sổ đó.
   Hãy chỉnh STABLE_DELTA_G theo nhiễu thật: khoảng 2.5 - 3 lần độ lệch chuẩn nhiễu. */
#define STABLE_SAMPLES          4
#define STABLE_DELTA_G          4
/* Nhấc vật ra: cần REMOVAL_SAMPLES mẫu liên tiếp < PRESENCE_G thì mới tính là xong 1 sản phẩm. */
#define REMOVAL_SAMPLES         3
/* Sau khi có kết quả, nếu khối lượng TĂNG thêm > REARM_DELTA_G trong REARM_SAMPLES mẫu
   liên tiếp => coi như thêm vật, đo lại. (Một cú va chạm đơn lẻ sẽ bị bỏ qua.) */
#define REARM_DELTA_G           15
#define REARM_SAMPLES           3

/* ---------------- TARE / hiệu chuẩn ---------------- */
/* Khi nhấn TARE hoặc CAL, firmware gom RAW_AVG_SAMPLES mẫu MỚI (~0.8 s ở 10 SPS),
   chỉ chấp nhận khi (max - min) <= CAPTURE_MAX_SPREAD_G, ngược lại gom lại từ đầu. */
#define RAW_AVG_SAMPLES         8
#define CAPTURE_MAX_SPREAD_G    10

/* Hệ số mặc định trước khi hiệu chuẩn (chỉ để có số, KHÔNG dùng cho kết quả QC). */
#define DEFAULT_COUNTS_PER_GRAM 1000.0f
#define CAL_DEFAULT_G           1000u
#define CAL_MIN_G               100u
#define CAL_MAX_G               4000u
#define CAL_STEP_G              5u

/* ---------------- Cài đặt QC (chỉ lưu trong RAM) ---------------- */
#define DEFAULT_TARGET_G        500u
#define TARGET_STEP_G           50
#define TARGET_MIN_G            50
#define TARGET_MAX_G            4500

#define DEFAULT_TOLERANCE_G     10u
#define TOLERANCE_STEP_G        5
#define TOLERANCE_MIN_G         5
#define TOLERANCE_MAX_G         500

/* ---------------- Thời gian ---------------- */
#define HX_TASK_PERIOD_MS       20u     /* chu kỳ HX711Task kiểm tra DOUT              */
#define QC_TASK_PERIOD_MS       20u     /* chu kỳ QCTask                               */
#define OLED_PERIOD_MS          250u    /* chu kỳ vẽ lại màn hình                       */
#define SENSOR_TIMEOUT_MS       800u    /* quá lâu không có mẫu mới => SENSOR ERROR     */
#define HEALTH_CHECK_MS         1000u   /* chu kỳ kiểm tra "các task còn sống"         */

#define BTN_DEBOUNCE_MS         20u
#define BTN_LONG_MS             1500u   /* giữ CAL / RESET PCS                          */

/* ---------------- Hiển thị ---------------- */
#define DISPLAY_DEADBAND_G      5       /* chống nhấp nháy số khi cân đứng yên         */

/* ---------------- Watchdog ---------------- */
/* 1 = bật IWDG (~2 s). Để 0 khi đang debug / chưa test phần cứng. */
#define ENABLE_IWDG             0

#endif /* APP_PARAMS_H */
