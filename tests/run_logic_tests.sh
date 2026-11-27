#!/usr/bin/env bash
# Chạy test logic trên PC (không cần HAL / FreeRTOS / board).
# Dùng đúng tham số trong Core/Inc/app_params.h.
set -euo pipefail
cd "$(dirname "$0")/.."
OUT="$(mktemp -d)"
FLAGS="-std=c99 -Wall -Wextra -Werror -ICore/Inc -Iapp/inc"

gcc $FLAGS tests/test_qc_logic.c app/src/state_machine.c app/src/scale.c -lm -o "$OUT/test_qc"
"$OUT/test_qc"

gcc $FLAGS tests/test_scale_logic.c app/src/scale.c -lm -o "$OUT/test_scale"
"$OUT/test_scale"
