import pytest

pytestmark = pytest.mark.anyio

DISABLED_REGEX = "Feature is not enabled."


async def test_sht40_show_temp(board):
    """
    Test 'sensors show sht_temp' command for both enabled and
    disabled cases.
    """

    is_enabled = board.config.get("CONFIG_APP_USE_SHT40") == "y"

    if is_enabled:
        expected = r"SHT40 temperature:.+\..+C"
    else:
        expected = DISABLED_REGEX

    await board.send_cmd("sensors show sht_temp")

    response = await board.wait_for_regex_in_line(expected)

    assert response is not None, \
           f"Expected '{expected}' in response, but got nothing."
