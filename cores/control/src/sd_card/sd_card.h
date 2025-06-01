#ifndef SD_CARD_H_
#define SD_CARD_H_

#ifdef CONFIG_SD_CARD_WRITER

#include <zephyr/kernel.h>

void sd_card_submit_line(const char *line,
                         size_t line_len,
                         k_timeout_t timeout);

int sd_writer_init(void);

#endif /* CONFIG_SD_CARD_WRITER */

#endif /* SD_CARD_H_ */
