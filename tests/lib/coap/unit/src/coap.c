#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/net/coap.h>
#include <string.h>
#include <stdint.h>

#include <lib/coap.h>

#define TEST_BUF_SIZE 256
static uint8_t buf[TEST_BUF_SIZE];

const char * const path[] = { "sensors", "temp", NULL };

ZTEST(coap_unit, test_get_no_path)
{
    int ret = coap_build_request(buf, sizeof(buf),
                                 COAP_TYPE_NON_CON,
                                 COAP_METHOD_GET,
                                 NULL,
                                 NULL, 0);

    zassert_true(ret > 0, "GET request with no path should succeed");
}

ZTEST(coap_unit, test_get_with_path)
{
    int ret = coap_build_request(buf, sizeof(buf),
                                 COAP_TYPE_NON_CON,
                                 COAP_METHOD_GET,
                                 path,
                                 NULL, 0);

    zassert_true(ret > 0, "GET request with path should succeed");
}

ZTEST(coap_unit, test_delete_with_path)
{
	int ret = coap_build_request(buf, sizeof(buf),
                                 COAP_TYPE_NON_CON,
                                 COAP_METHOD_DELETE,
                                 path,
                                 NULL, 0);

	zassert_true(ret > 0, "DELETE request with path should succeed");
}

ZTEST(coap_unit, test_post_no_payload)
{
    const uint8_t payload[] = { 0x01 };

    int ret = coap_build_request(buf, sizeof(buf),
                                 COAP_TYPE_NON_CON,
                                 COAP_METHOD_POST,
                                 path,
                                 payload, sizeof(payload));

    zassert_true(ret > 0, "POST request with no payload should succeed");
}

ZTEST(coap_unit, test_post_with_payload)
{
    const uint8_t payload[] = { 0x01 };

    int ret = coap_build_request(buf, sizeof(buf),
                                 COAP_TYPE_NON_CON,
                                 COAP_METHOD_POST,
                                 path,
                                 payload, sizeof(payload));

    zassert_true(ret > 0, "POST request with payload should succeed");
}

ZTEST(coap_unit, test_put_no_payload)
{
    const uint8_t payload[] = { 0x01 };

    int ret = coap_build_request(buf, sizeof(buf),
                                 COAP_TYPE_NON_CON,
                                 COAP_METHOD_PUT,
                                 path,
                                 payload, sizeof(payload));

    zassert_true(ret > 0, "PUT request with no payload should succeed");
}

ZTEST(coap_unit, test_put_with_payload)
{
    const uint8_t payload[] = { 0x01 };

    int ret = coap_build_request(buf, sizeof(buf),
                                 COAP_TYPE_NON_CON,
                                 COAP_METHOD_PUT,
                                 path,
                                 payload, sizeof(payload));

    zassert_true(ret > 0, "PUT request with payload should succeed");
}

ZTEST(coap_unit, test_invalid_method)
{
    int ret = coap_build_request(buf, sizeof(buf),
                                 COAP_TYPE_NON_CON,
                                 -1,
                                 NULL,
                                 NULL, 0);

    zassert_true(ret < 0, "Invalid method should not succeed");

    ret = coap_build_request(buf, sizeof(buf),
                                 COAP_TYPE_NON_CON,
                                 COAP_METHOD_IPATCH,
                                 NULL,
                                 NULL, 0);

    zassert_true(ret < 0, "Invalid method should not succeed");
}

ZTEST(coap_unit, test_invalid_type)
{
    int ret = coap_build_request(buf, sizeof(buf),
                                 COAP_TYPE_CON,
                                 COAP_METHOD_GET,
                                 NULL,
                                 NULL, 0);

    zassert_true(ret < 0, "Unsupported message type should fail");

    ret = coap_build_request(buf, sizeof(buf),
                                 -1,
                                 COAP_METHOD_GET,
                                 NULL,
                                 NULL, 0);

    zassert_true(ret < 0, "Invalid message type should fail");
}

ZTEST(coap_unit, test_null_buffer)
{
    int ret = coap_build_request(NULL, 100,
                                 COAP_TYPE_NON_CON,
                                 COAP_METHOD_GET,
                                 NULL,
                                 NULL, 0);

    zassert_true(ret < 0, "Null buffer should fail");
}

ZTEST(coap_unit, test_zero_buf_len)
{
    int ret = coap_build_request(buf, 0,
                                 COAP_TYPE_NON_CON,
                                 COAP_METHOD_GET,
                                 NULL,
                                 NULL, 0);

    zassert_true(ret < 0, "Zero buffer should fail");
}

ZTEST_SUITE(coap_unit, NULL, NULL, NULL, NULL, NULL);
