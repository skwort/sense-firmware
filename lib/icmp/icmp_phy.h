#ifndef _LIB_ICMP_PHY_H_
#define _LIB_ICMP_PHY_H_

#include <lib/icmp.h>

struct icmp_phy_api {
    int (*init)(void);
    int (*send)(struct icmp_frame *frame);
};

const struct icmp_phy_api *icmp_get_selected_phy(void);

#endif /* _LIB_ICMP_PHY_H_ */
