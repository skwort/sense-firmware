from .board import Board
from pynrfjprog import LowLevel


class NordicBoard(Board):
    def program(self, fw_image):
        with LowLevel.API() as api:
            api.connect_to_emu_with_snr(int(self.serial_number))
            api.erase_all()
            api.program_file(self.fw_image)
            api.sys_reset()
            api.go()
            api.close()
