#ifndef HX711_H
#define HX711_H
/*
 * Driver ADC 24 bit HX711 (bit-bang 2 dây: DOUT, SCK).
 *   - DOUT = 0  => có mẫu mới sẵn sàng.
 *   - Đọc 24 bit (MSB trước) bằng 24 xung SCK, thêm xung thứ 25 => kênh A, gain 128.
 */
#include <stdbool.h>
#include <stdint.h>

void HX711_Init(void);
bool HX711_IsReady(void);

/* Trả về false nếu chưa có mẫu, hoặc mẫu bão hòa (thường do đứt dây loadcell). */
bool HX711_Read(int32_t *raw);

#endif /* HX711_H */
