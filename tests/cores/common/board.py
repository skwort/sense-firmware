from abc import ABC, abstractmethod
from contextlib import asynccontextmanager
import re
import trio
from trio_serial import SerialStream


class Board(ABC):
    def __init__(self, port, baudrate, fw_image, config, serial_number=None):
        if serial_number is not None:
            self.serial_number = serial_number
        self.port = port
        self.baudrate = baudrate
        self.fw_image = fw_image
        self.config = config

        self.received: bytes = b""

        self.serial: SerialStream | None = None

    @asynccontextmanager
    async def started(self):
        if self.fw_image:
            self.program(self.fw_image)

        async with SerialStream(port=self.port,
                                baudrate=self.baudrate) as serial:
            self.serial = serial

            # Wait for reboot
            await trio.sleep(6)

            yield self

    async def receive_some(self) -> bytes:
        assert self.serial is not None

        return await self.serial.receive_some()

    async def send_all(self, data: bytes):
        assert self.serial is not None

        await self.serial.send_all(data)

    async def wait_for_regex_in_line(self, regex, timeout_s=20, log=True):
        with trio.fail_after(timeout_s):
            while True:
                # Check in already received data
                lines = self.received.splitlines(keepends=True)

                for idx, line in enumerate(lines):
                    regex_search = re.search(
                        regex,
                        line.replace(b"\r", b"")
                        .replace(b"\n", b"")
                        .decode("utf-8", errors="ignore"),
                    )
                    if regex_search:
                        # Drop this and any previous lines
                        self.received = b"".join(lines[idx + 1:])

                        return regex_search

                # Drop all but last line (in case it is not complete)
                if len(lines) > 1:
                    self.received = lines[-1]

                # Receive more data
                chunk = await self.receive_some()
                if chunk == b"":
                    return

                if log:
                    log_msg = chunk.replace(b"\r", b"")
                    print(log_msg.decode("utf-8", errors="ignore"), end="")

                self.received = self.received + chunk

    async def send_cmd(self, cmd, wait_str=None):
        if wait_str is None:
            wait_str = self.PROMPT
        await self.send_all("\r\n\r\n".encode())
        await self.wait_for_regex_in_line(self.PROMPT)
        await self.send_all("{}\r\n".format(cmd).encode())
        await self.wait_for_regex_in_line(wait_str)

    @property
    @abstractmethod
    def PROMPT(self):
        pass

    @abstractmethod
    async def reset(self):
        pass

    @abstractmethod
    def program(self, fw_image):
        pass
