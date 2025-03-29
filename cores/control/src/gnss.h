#ifndef GNSS_H_
#define GNSS_H_

#include "state.h"

#ifdef CONFIG_APP_USE_GNSS
int gnss_init(struct state *state);

void poll_gnss(struct state *state);
#endif /* CONFIG_APP_USE_GNSS */

#endif /* GNSS_H_ */
