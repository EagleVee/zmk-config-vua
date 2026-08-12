/*
 * Copyright (c) 2026 Deemen17
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_hold_reset

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>

#include <drivers/behavior.h>

#include <dt-bindings/zmk/reset.h>
#include <zmk/behavior.h>

#if IS_ENABLED(CONFIG_RETENTION_BOOT_MODE)

#include <zephyr/retention/bootmode.h>

#endif /* IS_ENABLED(CONFIG_RETENTION_BOOT_MODE) */

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct behavior_hold_reset_config {
    uint32_t hold_time_ms;
    /* 0 disables the second tier. */
    uint32_t bootloader_hold_time_ms;
#if IS_ENABLED(CONFIG_RETENTION_BOOT_MODE)
    enum BOOT_MODE_TYPES boot_mode;
#else
    int type;
#endif /* IS_ENABLED(CONFIG_RETENTION_BOOT_MODE) */
};

struct behavior_hold_reset_data {
    int64_t press_start;
};

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_hold_reset_data *data = dev->data;

    data->press_start = k_uptime_get();

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_hold_reset_data *data = dev->data;
    const struct behavior_hold_reset_config *cfg = dev->config;

    int64_t hold_time = k_uptime_get() - data->press_start;

    /*
     * The tier is picked on release rather than as soon as a threshold passes,
     * because firing at hold-time-ms would reset before the longer hold could
     * ever be reached. Releasing between the two thresholds is what selects the
     * shorter action.
     */
    bool to_bootloader =
        cfg->bootloader_hold_time_ms > 0 && hold_time >= cfg->bootloader_hold_time_ms;

    if (!to_bootloader && hold_time < cfg->hold_time_ms) {
        LOG_DBG("Not resetting: held for %lld ms, hold time is %u ms", hold_time,
                cfg->hold_time_ms);
        return ZMK_BEHAVIOR_OPAQUE;
    }

    LOG_WRN("Held for %lld ms, resetting%s", hold_time, to_bootloader ? " to the bootloader" : "");

#if IS_ENABLED(CONFIG_RETENTION_BOOT_MODE)
    int ret = bootmode_set(to_bootloader ? BOOT_MODE_TYPE_BOOTLOADER : cfg->boot_mode);
    if (ret < 0) {
        LOG_ERR("Failed to set the boot mode (%d)", ret);
        return ZMK_BEHAVIOR_OPAQUE;
    }

    sys_reboot(SYS_REBOOT_WARM);
#else
    sys_reboot(to_bootloader ? RST_UF2 : cfg->type);
#endif /* IS_ENABLED(CONFIG_RETENTION_BOOT_MODE) */

    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_hold_reset_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
    .locality = BEHAVIOR_LOCALITY_EVENT_SOURCE,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif /* IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA) */
};

#if IS_ENABLED(CONFIG_RETENTION_BOOT_MODE)
#define HR_BOOT_TARGET(n)                                                                          \
    .boot_mode = DT_INST_PROP(n, bootloader) ? BOOT_MODE_TYPE_BOOTLOADER : BOOT_MODE_TYPE_NORMAL
#else
#define HR_BOOT_TARGET(n) .type = DT_INST_PROP(n, type)
#endif /* IS_ENABLED(CONFIG_RETENTION_BOOT_MODE) */

#define HR_INST(n)                                                                                 \
    BUILD_ASSERT(DT_INST_PROP(n, bootloader_hold_time_ms) == 0 ||                                  \
                     DT_INST_PROP(n, bootloader_hold_time_ms) > DT_INST_PROP(n, hold_time_ms),     \
                 "bootloader-hold-time-ms must be greater than hold-time-ms");                     \
    static struct behavior_hold_reset_data behavior_hold_reset_data_##n = {};                      \
    static const struct behavior_hold_reset_config behavior_hold_reset_config_##n = {              \
        .hold_time_ms = DT_INST_PROP(n, hold_time_ms),                                             \
        .bootloader_hold_time_ms = DT_INST_PROP(n, bootloader_hold_time_ms),                       \
        HR_BOOT_TARGET(n),                                                                         \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, &behavior_hold_reset_data_##n,                          \
                            &behavior_hold_reset_config_##n, POST_KERNEL,                          \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                                   \
                            &behavior_hold_reset_driver_api);

DT_INST_FOREACH_STATUS_OKAY(HR_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
