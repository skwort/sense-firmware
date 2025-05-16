#ifndef _LIB_CELLULAR_BACKEND_H_
#define _LIB_CELLULAR_BACKEND_H_

struct cellular_backend {
    int (*init)(void);
};

const struct cellular_backend *cellular_get_selected_backend(void);

#endif /* _LIB_CELLULAR_BACKEND_H_ */
