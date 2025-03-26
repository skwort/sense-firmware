import pytest
import json
from .nrf9161dk import nRF9161DK


def pytest_addoption(parser):
    parser.addoption(
        "--board",
        type=str,
        help="The board being tested"
    )
    parser.addoption(
        "--port",
        help="The port to which the device is attached (eg: /dev/ttyACM0)"
    )
    parser.addoption(
        "--baud",
        type=int,
        default=115200,
        help="Serial port baud rate (default: 115200)"
    )
    parser.addoption(
        "--fw-image",
        type=str,
        help="Firmware binary to program to device"
    )
    parser.addoption(
        "--serial-number",
        type=str,
        help="Serial number to identify on-board debugger"
    )
    parser.addoption(
        "--build-map",
        type=str,
        help="The path to the build map."
    )


@pytest.fixture(scope="session")
def board_name(request):
    return request.config.getoption("--board")


@pytest.fixture(scope="session")
def port(request):
    return request.config.getoption("--port")


@pytest.fixture(scope="session")
def baud(request):
    return request.config.getoption("--baud")


@pytest.fixture(scope="session")
def fw_image(request):
    return request.config.getoption("--fw-image")


@pytest.fixture(scope="session")
def serial_number(request):
    return request.config.getoption("--serial-number")


@pytest.fixture(scope="session")
def build_map(request):
    return request.config.getoption("--build-map")


@pytest.fixture(scope="module")
async def board(board_name, port, baud, fw_image, serial_number, build_map):

    # Load the build map to extract the image config
    config = None
    try:
        with open(build_map) as f:
            firmware_images = json.load(f)["firmware_images"]

            # Match the fw_image to the the build_map entry
            for image in firmware_images:
                fw_image_name = fw_image.split("/")[-1]

                if fw_image_name.startswith(image["name"]):
                    config = image["config"]
                    break

    except FileNotFoundError:
        raise ValueError(f"Unable to find build map file: '{build_map}'")

    except json.JSONDecodeError:
        raise ValueError(f"Error decoding JSON in build map: '{build_map}'")

    if config is None:
        raise ValueError(
            f"Unable to locate config for '{fw_image}' in build map")

    if board_name.lower() == "nrf9161dk":
        board = nRF9161DK(port, baud, fw_image, config,
                          serial_number=serial_number)
    else:
        raise ValueError("Unknown board")

    async with board.started():
        yield board
