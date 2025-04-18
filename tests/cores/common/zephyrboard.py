from .board import Board


class ZephyrBoard(Board):
    @property
    def PROMPT(self):
        return 'uart:'

    async def set_setting(self, key, value):
        await self.send_cmd(f'settings set {key} {value}', wait_str='saved')

    async def reset(self):
        await self.send_cmd('kernel reboot cold', wait_str='Booting Zephyr OS')
