#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <string.h>

#include "sdi12_queue.h"

#define RESPONSE_TIMEOUT_MS 1000

static int cmd_sdi12(const struct shell *sh, size_t argc, char **argv)
{
    if (argc != 2) {
        shell_error(sh, "Usage: sdi12 <command>");
        return -EINVAL;
    }

    char buf[SDI12_MSG_LEN] = {0};
    size_t len = strlen(argv[1]);
    if (argv[1][len - 1] == '!') {
        snprintf(buf, SDI12_MSG_LEN, "%s", argv[1]);
    } else {
        snprintf(buf, SDI12_MSG_LEN, "%s!", argv[1]);
    }
    shell_print(sh, "Sending command: %s", buf);

    if (sdi12_command_enqueue(buf, K_NO_WAIT) < 0) {
        shell_error(sh, "Failed to queue command");
        return -EIO;
    }

    /* Wait for response */
    if (sdi12_response_dequeue(buf, K_MSEC(RESPONSE_TIMEOUT_MS)) < 0) {
        shell_error(sh, "Timed out waiting for response");
        return -ETIMEDOUT;
    }

    shell_print(sh, "Response: %s", buf);
    return 0;
}

SHELL_CMD_REGISTER(sdi12, NULL, "Send SDI-12 command (e.g., `sdi12 0M`)", cmd_sdi12);
