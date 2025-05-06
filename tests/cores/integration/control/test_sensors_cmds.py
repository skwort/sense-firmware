import pytest
from hypothesis import settings, given, strategies as st

pytestmark = pytest.mark.anyio

DISABLED_REGEX = "Feature is not enabled."

LL_INT_MAX = 9223372036854775807


@pytest.mark.parametrize(
    "cmd, output_if_enabled", [
        ("temp", r"SHT40 temperature:.+\..+C"),
        ("hum", r"SHT40 humidity:.+%"),
        ("poll_freq", r"SHT40 poll frequency:.+ms"),
    ]
)
async def test_sensors_show_sht(board, cmd, output_if_enabled):
    """
    Test 'sensors show sht_*' commands for both enabled and
    disabled cases.
    """

    is_enabled = board.config.get("CONFIG_APP_USE_SHT40") == "y"

    if is_enabled:
        expected = output_if_enabled
    else:
        expected = DISABLED_REGEX

    await board.send_cmd(f"sensors show sht_{cmd}")

    response = await board.wait_for_regex_in_line(expected)

    assert response is not None, \
           f"Expected '{expected}' in response, but got nothing."


@pytest.mark.parametrize(
    "cmd, output_if_enabled", [
        ("accel", r"IMU accelerometer:.+x:.+y:.+z:.+"),
        ("gyro", r"IMU gyroscope:.+x:.+y:.+z:.+"),
        ("mag", r"IMU magnetometer:.+x:.+y:.+z:.+"),
        ("poll_freq", r"IMU poll frequency:.+ms"),
    ]
)
async def test_sensors_show_imu(board, cmd, output_if_enabled):
    """
    Test 'sensors show imu_*' commands for both enabled and
    disabled cases.
    """

    is_enabled = board.config.get("CONFIG_APP_USE_IMU") == "y"

    if is_enabled:
        expected = output_if_enabled
    else:
        expected = DISABLED_REGEX

    await board.send_cmd(f"sensors show imu_{cmd}")

    response = await board.wait_for_regex_in_line(expected)

    assert response is not None, \
           f"Expected '{expected}' in response, but got nothing."


@pytest.mark.parametrize(
    "cmd", [
        ("sht"),
        ("imu")
    ]
)
@settings(max_examples=5)
@given(value=st.integers(min_value=1, max_value=LL_INT_MAX))
async def test_sensors_update_poll_freq_valid(board, cmd, value):
    """
    Test 'sensors update *_poll_freq NUM' commands for valid poll
    frequencies. Note that this test uses hypothesis for
    property-based testing.
    """

    if cmd == "imu":
        is_enabled = board.config.get("CONFIG_APP_USE_IMU") == "y"
    elif cmd == "sht":
        is_enabled = board.config.get("CONFIG_APP_USE_SHT40") == "y"
    else:
        raise ValueError(f"Unsupported parameter '{cmd}'.")

    # Expected return for update and show commands
    if is_enabled:
        expected = rf"{cmd.upper()}.* poll frequency: {str(value)} ms"
    else:
        expected = DISABLED_REGEX

    await board.send_cmd(f"sensors update {cmd}_poll_freq {str(value)}")

    response = await board.wait_for_regex_in_line(expected)

    assert response is not None, \
           f"Expected '{expected}' in response, but got nothing."

    # Validate internal state has been updated by executing `show`
    await board.send_cmd(f"sensors show {cmd}_poll_freq {str(value)}")

    response = await board.wait_for_regex_in_line(expected)

    assert response is not None, \
           f"Expected '{expected}' in response, but got nothing."


@pytest.mark.parametrize(
    "cmd, value", [
        ("sht", 0),
        ("sht", -1),
        ("sht", LL_INT_MAX + 1),
        ("imu", 0),
        ("imu", -1),
        ("imu", LL_INT_MAX + 1)
    ]
)
async def test_sensors_update_poll_freq_invalid(board, cmd, value):
    """
    Test 'sensors update *_poll_freq NUM' commands for invalid poll
    frequencies.
    """

    if cmd == "imu":
        is_enabled = board.config.get("CONFIG_APP_USE_IMU") == "y"
    elif cmd == "sht":
        is_enabled = board.config.get("CONFIG_APP_USE_SHT40") == "y"
    else:
        raise ValueError(f"Unsupported parameter '{cmd}'.")

    # Expected return for update cmd
    if is_enabled:
        expected = r"Invalid poll frequency."
    else:
        expected = DISABLED_REGEX

    await board.send_cmd(f"sensors update {cmd}_poll_freq {str(value)}")

    response = await board.wait_for_regex_in_line(expected)

    assert response is not None, \
           f"Expected '{expected}' in response, but got nothing."


@pytest.mark.parametrize(
    "cmd", [
        ("sht"),
        ("imu")
    ]
)
async def test_sensors_update_poll_freq_bad_args(board, cmd):
    """
    Test 'sensors update *_poll_freq NUM' commands for invalid poll
    frequencies
    """

    if cmd == "imu":
        is_enabled = board.config.get("CONFIG_APP_USE_IMU") == "y"
    elif cmd == "sht":
        is_enabled = board.config.get("CONFIG_APP_USE_SHT40") == "y"
    else:
        raise ValueError(f"Unsupported parameter '{cmd}'.")

    if is_enabled:
        expected = r"Usage:"
    else:
        expected = DISABLED_REGEX

    # Missing NUM argument
    await board.send_cmd(f"sensors update {cmd}_poll_freq")

    response = await board.wait_for_regex_in_line(expected)

    assert response is not None, \
           f"Expected '{expected}' in response, but got nothing."

    # Too many arguments
    await board.send_cmd(f"sensors update {cmd}_poll_freq extra1 extra2")

    response = await board.wait_for_regex_in_line(expected)

    assert response is not None, \
           f"Expected '{expected}' in response, but got nothing."
