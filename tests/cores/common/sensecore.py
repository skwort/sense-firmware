import trio
from .ncsboard import NCSBoard
from .nordicboard import NordicBoard
from .rtt import RTTStream
from contextlib import asynccontextmanager


class SenseCore(NCSBoard, NordicBoard):
    def __init__(self, fw_image, config, board, serial_number=None):
        if serial_number is not None:
            self.serial_number = serial_number
        self.fw_image = fw_image
        self.config = config
        self.board = board

        self.received: bytes = b""

        self.serial: RTTStream | None = None

    @asynccontextmanager
    async def started(self):
        if self.fw_image:
            self.program(self.fw_image)

        if self.board.endswith("nrf9161"):
            target_device = "nRF9160_xxAA"
        elif self.board.endswith("nrf5340"):
            target_device = "nRF5340_xxAA_APP"
        else:
            raise ValueError(f"Board '{self.board}' is not supported.")

        async with RTTStream(target_device=target_device) as stream:
            self.serial = stream

            await trio.sleep(6)

            yield self

    @property
    def PROMPT(self):
        return 'rtt:'
