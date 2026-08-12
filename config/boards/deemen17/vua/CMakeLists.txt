# SPDX-License-Identifier: MIT

# Custom hold-to-reset behavior, used by the kscan sideband entries in
# vua_left_nrf52840.dts / vua_right_nrf52840.dts. Built for both halves,
# including the peripheral, so each half can reset itself locally.
target_sources_ifdef(CONFIG_DT_HAS_ZMK_BEHAVIOR_HOLD_RESET_ENABLED app PRIVATE hold_reset.c)
