#include <zephyr/kernel.h>
#include <zephyr/net/coap.h>
#include <zephyr/logging/log.h>

#include <lib/coap.h>

LOG_MODULE_REGISTER(coap_lib, CONFIG_COAP_LIB_LOG_LEVEL);

int coap_build_request(uint8_t *buf, size_t buf_len,
                       enum coap_msgtype type,
                       enum coap_method method,
                       const char * const *uri_path,
                       const uint8_t *payload, size_t payload_len)
{
    int ret;
    struct coap_packet request;

    if (method < COAP_METHOD_GET || method > COAP_METHOD_DELETE) {
        LOG_ERR("Invalid method. Only GET PUT POST and DELETE are supported.");
        return -EINVAL;
    }

    if (type != COAP_TYPE_NON_CON) {
        LOG_ERR("Invalid msg type. Only NON-CON messages are supported.");
        return -EINVAL;
    }

    ret = coap_packet_init(&request, buf, buf_len,
                           COAP_VERSION_1, type,
                           COAP_TOKEN_MAX_LEN, coap_next_token(),
                           method, coap_next_id());
    if (ret != 0) {
        LOG_ERR("Failed to init CoAP message: %d", ret);
        return ret;
    }

    if (uri_path) {
        for (const char * const *p = uri_path; *p; ++p) {
            ret = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
                                            *p, strlen(*p));
            if (ret != 0) {
                LOG_ERR("Unable to add URI_PATH option: %d", ret);
                return ret;
            }
        }
    }

    if ((method == COAP_METHOD_PUT || method == COAP_METHOD_POST) &&
        (payload && payload_len > 0)) {
        ret = coap_packet_append_payload_marker(&request);
        if (ret != 0) {
            LOG_ERR("Failed to add payload marker: %d", ret);
            return ret;
        }

        ret = coap_packet_append_payload(&request, payload, payload_len);
        if (ret != 0) {
            LOG_ERR("Failed to append payload: %d", ret);
            return ret;
        }
    }

    return request.offset;
}
