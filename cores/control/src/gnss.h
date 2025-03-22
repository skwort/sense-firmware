#ifndef GNSS_H_
#define GNSS_H_

#include "state.h"

int gnss_init(struct state *state);

void poll_gnss(struct state *state);

#endif /* GNSS_H_ */
