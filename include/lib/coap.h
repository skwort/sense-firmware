#ifndef LIB_COAP_H_
#define LIB_COAP_H_

#ifdef CONFIG_COAP_LIB

#include <zephyr/kernel.h>
#include <zephyr/net/coap.h>

/**
 * @brief Build a simple NON-CON CoAP request.
 *
 * This function builds a CoAP message in the provided buffer using the given
 *  parameters.
 *
 * @param buf          Destination buffer to write the encoded CoAP message.
 * @param buf_len      Length of the destination buffer.
 * @param type         CoAP message type. Must be COAP_TYPE_NON_CON.
 * @param method       CoAP method code (COAP_METHOD_GET, PUT, POST, DELETE).
 * @param uri_path     NULL-terminated array of URI path components. Can be NULL.
 * @param payload      Payload pointer. Can be NULL if no payload.
 * @param payload_len  Length of the payload in bytes.
 *
 * @return >0 on success (number of bytes used), or negative errno code on failure.
 */
int coap_build_request(uint8_t *buf, size_t buf_len,
                       enum coap_msgtype type,
                       enum coap_method method,
                       const char * const *uri_path,
                       const uint8_t *payload, size_t payload_len);

#endif /* CONFIG_COAP_LIB */

#endif /* LIB_COAP_H_ */
