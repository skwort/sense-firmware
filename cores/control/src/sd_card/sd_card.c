#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/logging/log.h>
#include <zephyr/fs/fs.h>
#include <string.h>

#include <ff.h>

LOG_MODULE_REGISTER(sd_card, CONFIG_SD_CARD_WRITER_LOG_LEVEL);

#define DISK_DRIVE_NAME     "SD"
#define DISK_MOUNT_PT       "/"DISK_DRIVE_NAME":"
#define FILE_NAME           DISK_MOUNT_PT"/log.csv"
#define BUFFER_SIZE         1024

static FATFS fat_fs;

static struct fs_mount_t mp = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
    .mnt_point = DISK_MOUNT_PT,
};

static char buffer_a[BUFFER_SIZE];
static char buffer_b[BUFFER_SIZE];

static char *active_buf = buffer_a;
static char *flush_buf  = buffer_b;

static size_t active_pos = 0;
struct k_work sd_card_writer_work;
static struct k_mutex buf_mutex;

#define SD_CARD_NODE DT_NODELABEL(ls_sdcard)
static const struct gpio_dt_spec ls_sdcard =
    GPIO_DT_SPEC_GET(SD_CARD_NODE, gpios);

void sd_writer_work_handler(struct k_work *work)
{
    LOG_INF("Flush to SD started");

    gpio_pin_set_dt(&ls_sdcard, 1);
    k_sleep(K_MSEC(25));

	int res = fs_mount(&mp);
	if (res != 0) {
        LOG_ERR("Failed to mount: %d", res);
        return;
    }

    struct fs_file_t file;
    fs_file_t_init(&file);

    res = fs_open(&file, FILE_NAME, FS_O_CREATE | FS_O_WRITE | FS_O_APPEND);
	if (res != 0) {
        LOG_ERR("Failed to open file: %d", res);
        goto unmount;
    }

    size_t len = strnlen(flush_buf, BUFFER_SIZE);
    res = fs_write(&file, flush_buf, len);
    if (res < 0) {
        LOG_ERR("Failed to write to file: %d", res);
    }

    /* Note that fs_close() calls fs_sync() to flush caches */
    res = fs_close(&file);
	if (res != 0) {
		LOG_ERR("Failed to close file: %d", res);
    }

unmount:
	fs_unmount(&mp);
    if (res != 0) {
        LOG_ERR("Error unmounting disk: %d", res);
    }
    LOG_INF("Flush to SD done. Powering down.");
    k_sleep(K_MSEC(25));
    gpio_pin_set_dt(&ls_sdcard, 0);
}

int sd_card_submit_line(const char *line, size_t line_len, k_timeout_t timeout)
{
    int err = k_mutex_lock(&buf_mutex, timeout);
    if (err != 0) {
        LOG_ERR("Failed to acquire SD card mutex: %d", err);
        return err;
    }

    if (active_pos + line_len >= BUFFER_SIZE) {
        /* Null terminate active buffer then swap it out */
        active_buf[active_pos] = '\0';
        char *tmp = active_buf;
        active_buf = flush_buf;
        flush_buf = tmp;
        active_pos = 0;
        k_work_submit(&sd_card_writer_work);
    }

    memcpy(&active_buf[active_pos], line, line_len);
    active_pos += line_len;

    k_mutex_unlock(&buf_mutex);

    return 0;
}

int sd_writer_init(void)
{
    k_mutex_init(&buf_mutex);
    k_work_init(&sd_card_writer_work, sd_writer_work_handler);

    return 0;
}
