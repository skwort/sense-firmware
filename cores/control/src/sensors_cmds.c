#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

LOG_MODULE_REGISTER(sensors_cmds);

static int cmd_update_sht_poll_freq(const struct shell *sh, size_t argc,
                                    char **argv)
{
    LOG_INF("Updating SHT poll frequency.");
    return 0;
}

static int cmd_update_imu_poll_freq(const struct shell *sh, size_t argc,
                                    char **argv)
{
    LOG_INF("Updating IMU poll frequency.");
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
    LOG_INF("Showing SHT poll frequency.");
    return 0;
}

static int cmd_show_sht_temp(const struct shell *sh, size_t argc,
                             char **argv)
{
    LOG_INF("Showing SHT temperature.");
    return 0;
}

static int cmd_show_sht_hum(const struct shell *sh, size_t argc,
                            char **argv)
{
    LOG_INF("Showing SHT humidity.");
    return 0;
}

static int cmd_show_imu_poll_freq(const struct shell *sh, size_t argc,
                                  char **argv)
{
    LOG_INF("Showing IMU poll frequency.");
    return 0;
}

static int cmd_show_imu_accel(const struct shell *sh, size_t argc,
                              char **argv)
{
    LOG_INF("Showing IMU accelerometer XYZ values.");
    return 0;
}

static int cmd_show_imu_gyro(const struct shell *sh, size_t argc,
                             char **argv)
{
    LOG_INF("Showing IMU gyroscope XYZ values.");
    return 0;
}

static int cmd_show_imu_mag(const struct shell *sh, size_t argc,
                            char **argv)
{
    LOG_INF("Showing IMU magnetometer XYZ values.");
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
