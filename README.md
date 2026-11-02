# BTL_NHUNG

Khung dự án MCU theo tài liệu **QuyTrinh-Git-2.pdf**. Hiện chưa có mã
chương trình, FreeRTOS, cấu hình board hoặc công cụ build.

## Cấu trúc thư mục

```text
BTL_NHUNG/
├── bsp/                 # Board support và pin mapping
├── hal/                 # Driver ngoại vi (Hardware Abstraction Layer)
│   ├── inc/             # Header HAL
│   └── src/             # Mã nguồn HAL
├── middlewares/         # Sẽ thêm FreeRTOS Kernel, Port và Heap
├── app/                 # FreeRTOS Tasks và IPC
│   ├── inc/             # Header task, app_ipc.h, app_cfg.h
│   └── src/             # Mã nguồn task và IPC
├── main.c               # Chỗ đặt khởi tạo hệ thống sau này
├── .gitignore
└── README.md
```

Các file `.gitkeep` giữ thư mục rỗng trong Git; có thể xóa khi thêm file thật.

## Quy trình Git

- `main`: phiên bản ổn định chạy trên kit; sau lần khởi tạo, không commit trực tiếp.
- `develop`: nhánh tích hợp chính.
- `feature/<ten-task>`: tạo từ `develop`, phát triển riêng và mở PR về `develop`.
- `integration/<phien-ban>`: ghép nối hệ thống trước khi phát hành.

### Đưa khung lên lần đầu

Chạy tại thư mục gốc dự án khi sẵn sàng xuất bản khung:

```bash
git add README.md .gitignore main.c bsp/ hal/ middlewares/ app/
git commit -m "chore: initialize MCU project structure"
git branch -M main
git push -u origin main
git checkout -b develop
git push -u origin develop
```

### Thêm code sau này

```bash
git checkout develop
git pull --ff-only origin develop
git checkout -b feature/hal-gpio-uart
# Thêm code, kiểm thử trên board, commit và push nhánh feature.
# Mở Pull Request về develop để review và merge.
```

Thống nhất API HAL và IPC trước khi chia việc. Một người phụ trách cấu hình
chân, clock, FreeRTOS và sinh code từ IDE. Khi có board/toolchain cụ thể,
bổ sung hướng dẫn build, nạp firmware và kiểm thử tại đây.

## Quy ước nhóm

- Task nằm trong `app/src/`, header tương ứng trong `app/inc/`.
- Chỉ người phụ trách tích hợp chỉnh sửa `main.c`.
- ISR dùng API FreeRTOS có đuôi `FromISR`; task dùng `vTaskDelay()` hoặc
  `vTaskDelayUntil()` thay cho delay phần cứng.
- Không commit build output hoặc cấu hình IDE cá nhân.
- Commit chức năng dùng `feat(...)`, sửa lỗi dùng `fix(...)`, tái cấu trúc dùng
  `refactor(...)`.

Chưa có lệnh build/test vì đây chỉ là khung project, chưa có code và phụ thuộc.
