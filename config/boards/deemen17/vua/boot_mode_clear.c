/*
 * Copyright (c) 2026 Deemen17
 * SPDX-License-Identifier: MIT
 */

/*
 * Clear a stale bootloader boot mode at startup.
 *
 * &bootloader works by writing a magic value into GPREGRET1 and resetting; the
 * bootloader is then expected to consume that value and clear it. Not all of
 * them do -- the 0.5.0 nRF52840 DK UF2 build on this keyboard leaves it set, so
 * every later reset, including a press of the physical reset button, re-enters
 * the bootloader instead of the firmware.
 *
 * If this code is running then the bootloader has already made its decision and
 * chosen to start us, so whatever is left in the register is stale and safe to
 * clear. Runs after RETENTION_INIT_PRIORITY (86) so the retention device is
 * ready.
 */

#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/retention/bootmode.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static int clear_stale_boot_mode(void) {
    int ret = bootmode_clear();

    if (ret < 0) {
        LOG_WRN("Failed to clear the stale boot mode (%d)", ret);
    }

    return 0;
}

SYS_INIT(clear_stale_boot_mode, POST_KERNEL, 90);
