#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
gcc -std=c99 -Wall -Wextra -Werror -DAPP_CONFIG_H \
 -DPHYSICAL_OVERLOAD_G=4900 -DPRESENCE_G=20 -DSTABLE_DELTA_G=3 \
 -DSTABLE_SAMPLES=4 -DREMOVAL_SAMPLES=3 -DREARM_DELTA_G=15 \
 -ICore/Inc -Iapp/inc tests/test_qc_logic.c app/src/state_machine.c \
 -o /tmp/test_qc_logic
/tmp/test_qc_logic
gcc -std=c99 -Wall -Wextra -Werror -DAPP_CONFIG_H \
 -DCAL_MIN_G=100 -DCAL_MAX_G=4000 -DDEFAULT_COUNTS_PER_GRAM=1000.0f -DSCALE_CAPACITY_G=5000 \
 -ICore/Inc -Iapp/inc tests/test_scale_logic.c app/src/scale.c -lm \
 -o /tmp/test_scale_logic
/tmp/test_scale_logic
