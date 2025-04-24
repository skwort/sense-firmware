#include <zephyr/ztest.h>
#include <lib/icmp.h>
#include "icmp_frame.h"

/* Fixture for generating a valid frame */
static struct icmp_frame valid_frame(void)
{
	struct icmp_frame frame = {
		.type = ICMP_TYPE_COMMAND,
		.msg_id = 0x01,
		.target = 0x01,
		.length = 5,
		.payload = { 'H', 'e', 'l', 'l', 'o' }
	};
	return frame;
}

ZTEST(icmp_frame, test_pack_and_unpack_basic)
{
	uint8_t buf[ICMP_MAX_FRAME_SIZE];
	struct icmp_frame original = valid_frame();
	struct icmp_frame unpacked = {0};

    /* Pack the frame */
	int packed_len = icmp_frame_pack(&original, buf, sizeof(buf));
	zassert_true(packed_len > 0, "Packing failed with error %d", packed_len);

    /* Unpack the packed frame */
	int ret = icmp_frame_unpack(&unpacked, buf, packed_len);
	zassert_equal(ret, 0, "Unpacking failed");

    /* Validate each of the fields */
	zassert_equal(original.type, unpacked.type);
	zassert_equal(original.msg_id, unpacked.msg_id);
	zassert_equal(original.target, unpacked.target);
	zassert_equal(original.length, unpacked.length);
	zassert_mem_equal(unpacked.payload, original.payload, original.length);
}

ZTEST(icmp_frame, test_pack_frame_null)
{
	uint8_t buf[ICMP_MAX_FRAME_SIZE];

	int ret = icmp_frame_pack(NULL, buf, sizeof(buf));
	zassert_equal(ret, -EINVAL, "Expected EINVAL for NULL frame");
}

ZTEST(icmp_frame, test_pack_buffer_null)
{
	struct icmp_frame frame = valid_frame();

	int ret = icmp_frame_pack(&frame, NULL, 100);
	zassert_equal(ret, -EINVAL, "Expected EINVAL for NULL buffer");
}

ZTEST(icmp_frame, test_unpack_frame_null)
{
	uint8_t buf[ICMP_MAX_FRAME_SIZE];

	int ret = icmp_frame_unpack(NULL, buf, sizeof(buf));
	zassert_equal(ret, -EINVAL, "Expected EINVAL for NULL frame");
}

ZTEST(icmp_frame, test_unpack_buffer_null)
{
	struct icmp_frame frame = valid_frame();

	int ret = icmp_frame_unpack(&frame, NULL, 100);
	zassert_equal(ret, -EINVAL, "Expected EINVAL for NULL buffer");
}

ZTEST(icmp_frame, test_pack_payload_too_big)
{
	struct icmp_frame frame = {
		.type = ICMP_TYPE_COMMAND,
		.msg_id = 0x01,
		.target = 0x01,
		.length = ICMP_MAX_PAYLOAD_SIZE + 1,
		.payload = {0}
	};

	uint8_t buf[ICMP_MAX_FRAME_SIZE];

	int ret = icmp_frame_pack(&frame, buf, sizeof(buf));
	zassert_equal(ret, -EINVAL, "Expected EINVAL for oversized payload.");
}

ZTEST(icmp_frame, test_unpack_payload_too_big)
{
	uint8_t buf[ICMP_MAX_FRAME_SIZE];
	struct icmp_frame temp_frame = valid_frame();

	int ret = icmp_frame_pack(&temp_frame, buf, sizeof(buf));
	zassert_true(ret > 0, "Packing failed with error %d", ret);

    /* Spoof the payload len */
    buf[3] = ICMP_MAX_PAYLOAD_SIZE + 1;

	struct icmp_frame frame = {0};
	ret = icmp_frame_unpack(&frame, buf, sizeof(buf));
	zassert_equal(ret, -EINVAL, "Expected EINVAL for oversized payload.");
}

ZTEST(icmp_frame, test_pack_buffer_too_small)
{
	uint8_t buf[4];
	struct icmp_frame frame = valid_frame();

	int ret = icmp_frame_pack(&frame, buf, sizeof(buf));
	zassert_equal(ret, -ENOBUFS, "Expected ENOBUFS for small buffer.");
}

ZTEST(icmp_frame, test_unpack_buffer_too_small)
{
	uint8_t buf[3] = { 0x01, 0x02, 0x03 };
	struct icmp_frame unpacked;

	int ret = icmp_frame_unpack(&unpacked, buf, sizeof(buf));
	zassert_equal(ret, -EINVAL, "Expected EINVAL for invalid buffer");
}

ZTEST(icmp_frame, test_pack_invalid_type)
{
	uint8_t buf[ICMP_MAX_FRAME_SIZE];
	struct icmp_frame frame = valid_frame();
    frame.type = ICMP_TYPE_INVALID;

	int ret = icmp_frame_pack(&frame, buf, sizeof(buf));
	zassert_equal(ret, -EINVAL, "Expected EINVAL for invalid type");
}

ZTEST(icmp_frame, test_unpack_invalid_type)
{
	uint8_t buf[ICMP_MAX_FRAME_SIZE];
	struct icmp_frame frame = valid_frame();
	int ret = icmp_frame_pack(&frame, buf, sizeof(buf));
	zassert_true(ret > 0, "Packing failed with error %d", ret);

    buf[0] = ICMP_TYPE_INVALID;

	ret = icmp_frame_unpack(&frame, buf, sizeof(buf));
	zassert_equal(ret, -EINVAL, "Expected EINVAL for invalid type");
}

ZTEST(icmp_frame, test_unpack_invalid_crc)
{
	uint8_t buf[ICMP_MAX_FRAME_SIZE];
	struct icmp_frame original = valid_frame();
	int packed_len = icmp_frame_pack(&original, buf, sizeof(buf));

	/* Corrupt the CRC and fail to unpack */
	buf[packed_len - 1] ^= 0xFF;
	struct icmp_frame unpacked = {0};
	int ret = icmp_frame_unpack(&unpacked, buf, packed_len);
	zassert_equal(ret, -EINVAL, "Expected EINVAL for bad CRC");
}


ZTEST(icmp_frame, test_zero_length_payload)
{
	struct icmp_frame frame = {
		.type = ICMP_TYPE_NOTIFY,
		.msg_id = 0x10,
		.target = 0x01,
		.length = 0,
	};

	uint8_t buf[ICMP_MAX_FRAME_SIZE];
	struct icmp_frame unpacked = {0};

	int packed_len = icmp_frame_pack(&frame, buf, sizeof(buf));
	zassert_true(packed_len > 0, "Packing failed");

	int ret = icmp_frame_unpack(&unpacked, buf, packed_len);
	zassert_equal(ret, 0, "Unpacking failed");

	zassert_equal(unpacked.length, 0);
}

ZTEST(icmp_frame, test_max_payload)
{
	struct icmp_frame frame = {
		.type = ICMP_TYPE_COMMAND,
		.msg_id = 0x20,
		.target = 0x03,
		.length = ICMP_MAX_PAYLOAD_SIZE
	};

	for (int i = 0; i < frame.length; i++) {
		frame.payload[i] = i;
	}

	uint8_t buf[ICMP_MAX_FRAME_SIZE];
	struct icmp_frame unpacked = {0};

	int packed_len = icmp_frame_pack(&frame, buf, sizeof(buf));
	zassert_true(packed_len > 0, "Packing failed");

	int ret = icmp_frame_unpack(&unpacked, buf, packed_len);
	zassert_equal(ret, 0, "Unpacking failed");

	zassert_mem_equal(unpacked.payload, frame.payload, frame.length);
}

ZTEST(icmp_frame, test_frame_alloc_and_free)
{
    struct icmp_frame *frame;
    int ret = icmp_frame_alloc(&frame);
    zassert_true(ret == 0, "Frame alloc failed");
    icmp_frame_free(frame);
}

ZTEST_SUITE(icmp_frame, NULL, NULL, NULL, NULL, NULL);
