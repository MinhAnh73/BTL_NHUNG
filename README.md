# BTL_NHUNG — STM32 QC Scale

Khung thư mục dự án cân QC dùng STM32F103C8T6 và FreeRTOS, đối chiếu theo
`STM32_QC_Scale_FreeRTOS_CAL_LinhHoat_EEPROM.zip` và quy trình trong
`QuyTrinh-Git-2.pdf`.

**Đã bổ sung nền tảng chung** từ ZIP: toàn bộ `middlewares/`, `Core/Inc/`,
`app/inc/` và `app/src/app_ipc.c`. Hai người dùng chung header và IPC để triển khai
hai nhóm task HX711/Storage và OLED/Buzzer trên các nhánh riêng.

Driver tự viết, phần triển khai logic cân và bốn task, project Keil và mã khởi tạo
hệ thống sẽ được bổ sung trong các PR tiếp theo. `Core/Src/main.c` hiện vẫn chỉ chứa
ghi chú. Chưa thể build firmware hoàn chỉnh hoặc nạp kit từ phần nền tảng này.

## Cấu trúc thư mục

```text
BTL_NHUNG/
├── Core/
│   ├── Inc/                 # main.h, FreeRTOSConfig.h, cấu hình MCU
│   └── Src/                 # main.c, ngắt, MSP, hooks và startup
├── bsp/                     # Board support, sơ đồ chân (pinmap.md)
├── hal/
│   ├── inc/                 # Header driver tự viết
│   └── src/                 # HX711, OLED, buzzer, button, EEPROM 25LC256
├── app/
│   ├── inc/                 # Header task, cấu hình, kiểu dữ liệu và IPC
│   └── src/                 # Task, logic cân và máy trạng thái
├── middlewares/
│   ├── FreeRTOS-Kernel/
│   │   ├── include/
│   │   └── portable/
│   │       ├── GCC/ARM_CM3/
│   │       ├── RVDS/ARM_CM3/
│   │       └── MemMang/
│   └── STM32CubeF1/
│       └── Drivers/
│           ├── CMSIS/
│           │   ├── Include/
│           │   └── Device/ST/STM32F1xx/
│           │       ├── Include/
│           │       └── Source/Templates/arm/
│           └── STM32F1xx_HAL_Driver/
│               ├── Inc/Legacy/
│               └── Src/Legacy/
├── MDK-ARM/                 # Sẽ thêm project Keil và scatter file
├── tests/                   # Sẽ thêm kiểm thử logic chạy trên máy tính
├── .gitignore
└── README.md
```

Các file `.gitkeep` giữ thư mục rỗng trong Git; có thể xóa khi thêm file thật.
Giữ nguyên chữ hoa/thường của đường dẫn như trong ZIP. Dùng gốc repo hiện tại,
không cần thêm lớp thư mục `STM32_QC_Scale/` bên ngoài.

## Vị trí module khi bổ sung code

| Thành phần | Nơi đặt file sau này |
| --- | --- |
| Khởi tạo hệ thống | `Core/Src/main.c` (entrypoint duy nhất) |
| Ngắt, MSP, RTOS hooks, startup | `Core/Src/` |
| FreeRTOSConfig.h, app_config.h, cấu hình STM32 HAL | `Core/Inc/` |
| Sơ đồ chân | `bsp/pinmap.md` |
| HX711, OLED, buzzer, nút nhấn, EEPROM | `hal/inc/` và `hal/src/` |
| task_hx711, task_oled, task_buzzer, task_storage | `app/inc/` và `app/src/` |
| app_tasks, app_ipc, scale, state_machine | `app/inc/` và `app/src/` |
| app_cfg.h, app_types.h | `app/inc/` |
| FreeRTOS Kernel, port Cortex-M3, heap_4 | `middlewares/FreeRTOS-Kernel/` |
| STM32 HAL và CMSIS của ST | `middlewares/STM32CubeF1/Drivers/` |
| STM32_QC_Scale.uvprojx và scatter file | `MDK-ARM/` |
| test_scale_logic.c, test_qc_logic.c, script chạy test | `tests/` |

`hal/` chứa driver nhóm tự viết; HAL của ST nằm trong `middlewares/`.
Bảng mô tả vị trí module của dự án hoàn chỉnh. Hiện chỉ các file thuộc phần nền tảng
chung nêu ở đầu README đã được đưa vào repo; các module còn lại sẽ bổ sung sau.
Dự án ZIP hướng tới Keil µVision 5; phiên bản compiler và target sẽ được đối chiếu
trực tiếp với project Keil khi bổ sung code. Chưa thể build hoặc chạy test từ khung này.

## Quy trình Git

- `main`: phiên bản ổn định chạy trên kit; sau lần khởi tạo, không commit trực tiếp.
- `develop`: nhánh tích hợp chính.
- `feature/<ten-task>`: tạo từ `develop`, phát triển riêng và mở PR về `develop`.
- `integration/<phien-ban>`: ghép nối hệ thống trước khi phát hành.

### Thêm code sau này

```bash
git checkout develop
git pull --ff-only origin develop
git checkout -b feature/hal-hx711
# Thêm code, kiểm thử trên board, commit và push nhánh feature.
# Mở Pull Request về develop để review và merge.
```

Thống nhất API HAL và IPC trước khi chia việc. Một người phụ trách cấu hình
chân, clock, FreeRTOS và sinh code từ IDE. Khi đưa code vào, bổ sung hướng dẫn build Keil, nạp firmware và kiểm thử tại đây.

## Quy ước nhóm

- Task nằm trong `app/src/`, header tương ứng trong `app/inc/`.
- Chỉ người phụ trách tích hợp chỉnh sửa `Core/Src/main.c`.
- ISR dùng API FreeRTOS có đuôi `FromISR`; task dùng `vTaskDelay()` hoặc
  `vTaskDelayUntil()` thay cho delay phần cứng.
- Không commit build output hoặc cấu hình IDE cá nhân.
- Commit chức năng dùng `feat(...)`, sửa lỗi dùng `fix(...)`, tái cấu trúc dùng
  `refactor(...)`.

Chưa có lệnh build/test cho firmware hoàn chỉnh vì phần triển khai ứng dụng và project
Keil chưa được tích hợp. Phần nền tảng được đối chiếu nội dung với ZIP; chưa kiểm tra
build bằng Keil hoặc chạy trên phần cứng.
