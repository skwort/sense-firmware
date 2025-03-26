#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include "state.h"

LOG_MODULE_REGISTER(sensors_cmds);

#define UNSUPPORTED_FEATURE_STRING "Feature is not enabled."

/* Macro for early return in shell fns if features are not supported. */
#define RETURN_IF_FEATURE_DISABLED(CONFIG_FEATURE) \
    if (!IS_ENABLED(CONFIG_FEATURE)) { \
        shell_print(sh, UNSUPPORTED_FEATURE_STRING); \
        return 1; \
    }

/* We use a timeout to avoid blocking the shell thread indefinitely, improving
 * the user experience when the system is under heavy load. */
#define SHELL_STATE_LOCK_TIMEOUT   100
#define STATE_LOCK_FAILURE_STRING  "Resource is temporarily unavailable. " \
                                   "Try again shortly."

/* Macro for early return in shell fns if the state mutex is unavailable. */
#define RETURN_IF_STATE_LOCK_FAILED() \
    if (state_lock(K_MSEC(SHELL_STATE_LOCK_TIMEOUT)) != 0) { \
        shell_print(sh, STATE_LOCK_FAILURE_STRING); \
        return 1; \
    }

static int64_t parse_poll_freq(const struct shell *sh, size_t argc,
                               char **argv)
{
    if (argc != 2) {
        shell_print(sh,
                    "Usage:\n"
                    "  sensors update %s freq", argv[0]);
        return -1;
    }

    errno = 0;

    int64_t poll_freq = strtoll(argv[1], NULL, 10);
    if (errno || poll_freq < 1) {
        shell_print(sh, "Invalid poll frequency. Value must be in the range "
                        "1-%lld ms.", LLONG_MAX);
        return -1;
    }

    return poll_freq;
}

static int cmd_update_sht_poll_freq(const struct shell *sh, size_t argc,
                                    char **argv)
{
    RETURN_IF_FEATURE_DISABLED(CONFIG_APP_USE_SHT40);

#ifdef CONFIG_APP_USE_SHT40
    int64_t poll_freq = parse_poll_freq(sh, argc, argv);
    if (poll_freq == -1) {
        return 1;
    }

    RETURN_IF_STATE_LOCK_FAILED();

    state_get()->sht_poll_freq = poll_freq;

    state_unlock();

    shell_print(sh, "New SHT40 poll frequency: %lld ms", poll_freq);
#endif /* CONFIG_APP_USE_SHT40 */

    return 0;
}

static int cmd_update_imu_poll_freq(const struct shell *sh, size_t argc,
                                    char **argv)
{
    RETURN_IF_FEATURE_DISABLED(CONFIG_APP_USE_IMU);

#ifdef CONFIG_APP_USE_IMU
    int64_t poll_freq = parse_poll_freq(sh, argc, argv);
    if (poll_freq == -1) {
        return 1;
    }

    RETURN_IF_STATE_LOCK_FAILED();

    state_get()->imu_poll_freq = poll_freq;

    state_unlock();

    shell_print(sh, "New IMU poll frequency: %lld ms", poll_freq);
#endif /* CONFIG_APP_USE_IMU*/

    return 0;
}

/* Create the `sensors update` command set */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_sensors_update_cmds,
    SHELL_CMD(sht_poll_freq, NULL, "Update SHT40 poll frequency.", cmd_update_sht_poll_freq),
    SHELL_CMD(imu_poll_freq, NULL, "Update IMU poll frequency.", cmd_update_imu_poll_freq),
	SHELL_SUBCMD_SET_END
);

static int cmd_show_sht_poll_freq(const struct shell *sh, size_t argc,
                                  char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    RETURN_IF_FEATURE_DISABLED(CONFIG_APP_USE_SHT40);

#ifdef CONFIG_APP_USE_SHT40
    RETURN_IF_STATE_LOCK_FAILED();

    int64_t sht_poll_freq = state_get()->sht_poll_freq;

    state_unlock();

    shell_print(sh, "SHT40 poll frequency: %lld ms", sht_poll_freq);
#endif /* CONFIG_APP_USE_SHT40 */

    return 0;
}

static int cmd_show_sht_temp(const struct shell *sh, size_t argc,
                             char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    RETURN_IF_FEATURE_DISABLED(CONFIG_APP_USE_SHT40);

#ifdef CONFIG_APP_USE_SHT40
    RETURN_IF_STATE_LOCK_FAILED();

    double sht_temp = state_get()->sht_temp;

    state_unlock();

    shell_print(sh, "SHT40 temperature: %0.2f C", sht_temp);
#endif /* CONFIG_APP_USE_SHT40 */

    return 0;
}

static int cmd_show_sht_hum(const struct shell *sh, size_t argc,
                            char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    RETURN_IF_FEATURE_DISABLED(CONFIG_APP_USE_SHT40);

#ifdef CONFIG_APP_USE_SHT40
    RETURN_IF_STATE_LOCK_FAILED();

    double sht_hum = state_get()->sht_hum;

    state_unlock();

    shell_print(sh, "SHT40 humidity: %0.2f %%", sht_hum);
#endif /* CONFIG_APP_USE_SHT40 */

    return 0;
}

static int cmd_show_imu_poll_freq(const struct shell *sh, size_t argc,
                                  char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    RETURN_IF_FEATURE_DISABLED(CONFIG_APP_USE_IMU);

#ifdef CONFIG_APP_USE_IMU
    RETURN_IF_STATE_LOCK_FAILED();

    int64_t imu_poll_freq = state_get()->imu_poll_freq;

    state_unlock();

    shell_print(sh, "IMU poll frequency: %lld ms", imu_poll_freq);
#endif /* CONFIG_APP_USE_IMU */

    return 0;
}

static int cmd_show_imu_accel(const struct shell *sh, size_t argc,
                              char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    RETURN_IF_FEATURE_DISABLED(CONFIG_APP_USE_IMU);

#ifdef CONFIG_APP_USE_IMU
    RETURN_IF_STATE_LOCK_FAILED();

    struct state *state = state_get();

    double x = state->imu_accel_x;
    double y = state->imu_accel_y;
    double z = state->imu_accel_z;

    state_unlock();

    shell_print(sh, "IMU accelerometer: x: %0.2f  y: %0.2f  z: %0.2f", x, y, z);
#endif /* CONFIG_APP_USE_IMU */

    return 0;
}

static int cmd_show_imu_gyro(const struct shell *sh, size_t argc,
                             char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    RETURN_IF_FEATURE_DISABLED(CONFIG_APP_USE_IMU);

#ifdef CONFIG_APP_USE_IMU
    RETURN_IF_STATE_LOCK_FAILED();

    struct state *state = state_get();

    double x = state->imu_gyro_x;
    double y = state->imu_gyro_y;
    double z = state->imu_gyro_z;

    state_unlock();

    shell_print(sh, "IMU gyroscope: x: %0.2f  y: %0.2f  z: %0.2f", x, y, z);
#endif /* CONFIG_APP_USE_IMU */

    return 0;
}

static int cmd_show_imu_mag(const struct shell *sh, size_t argc,
                            char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    RETURN_IF_FEATURE_DISABLED(CONFIG_APP_USE_IMU);

#ifdef CONFIG_APP_USE_IMU
    RETURN_IF_STATE_LOCK_FAILED();

    struct state *state = state_get();

    double x = state->imu_mag_x;
    double y = state->imu_mag_y;
    double z = state->imu_mag_z;

    state_unlock();

    shell_print(sh, "IMU magnetometer: x: %0.2f  y: %0.2f  z: %0.2f", x, y, z);
#endif /* CONFIG_APP_USE_IMU */

    return 0;
}

/* Create the `sensors show` command set. */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_sensors_show_cmds,
    SHELL_CMD(sht_poll_freq, NULL, "Show SHT40 poll frequency.", cmd_show_sht_poll_freq),
    SHELL_CMD(sht_temp, NULL, "Show current SHT40 recorded temperature.", cmd_show_sht_temp),
    SHELL_CMD(sht_hum, NULL, "Show current SHT40 recorded humidity.", cmd_show_sht_hum),
    SHELL_CMD(imu_poll_freq, NULL, "Show IMU poll frequency.", cmd_show_imu_poll_freq),
    SHELL_CMD(imu_accel, NULL, "Show current IMU accelerometer XYZ values.", cmd_show_imu_accel),
    SHELL_CMD(imu_gyro, NULL, "Show current IMU gyroscope XYZ values.", cmd_show_imu_gyro),
    SHELL_CMD(imu_mag, NULL, "Show current IMU magnetometer XYZ values.", cmd_show_imu_mag),
	SHELL_SUBCMD_SET_END
);

/* Attach the `show` and `update` command sets to the `sensors` command. */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_sensors,
	SHELL_CMD(update, &sub_sensors_update_cmds, "Update sensor configurations", NULL),
	SHELL_CMD(show, &sub_sensors_show_cmds, "Show sensor values.", NULL),
	SHELL_SUBCMD_SET_END
);

/* Register the `sensors` root command. */
SHELL_CMD_REGISTER(sensors, &sub_sensors, "Commands for sensors module", NULL);
