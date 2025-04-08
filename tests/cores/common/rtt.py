import pylink
import trio


class RTTStream:
    def __init__(self, target_device, block_address=None):
        self.jlink = pylink.JLink()
        self.target_device = target_device
        self.block_address = block_address
        self._read_lock = trio.Lock()
        self._write_lock = trio.Lock()
        self._stop_polling = trio.Event()
        self._polling_ended = trio.Event()
        self._send_channel, self._recv_channel = trio.open_memory_channel(100)

    async def __aenter__(self):
        await trio.to_thread.run_sync(self.jlink.open)
        await trio.to_thread.run_sync(self.jlink.set_tif,
                                      pylink.enums.JLinkInterfaces.SWD)
        await trio.to_thread.run_sync(self.jlink.connect, self.target_device)

        await trio.to_thread.run_sync(self.jlink.rtt_start, self.block_address)

        self._nursery_manager = trio.open_nursery()
        self._nursery = await self._nursery_manager.__aenter__()
        self._nursery.start_soon(self._poll_rtt_read)

        return self

    async def __aexit__(self, exc_type, exc, tb):
        self._stop_polling.set()

        await self._polling_ended.wait()

        await trio.to_thread.run_sync(self.jlink.rtt_stop)
        await trio.to_thread.run_sync(self.jlink.close)
        self._nursery.cancel_scope.cancel()
        await self._nursery_manager.__aexit__(exc_type, exc, tb)

    async def _poll_rtt_read(self):
        """
        Continously poll the RTT interface and send data to the trio
        send_channel if data is available. Polling will continue until
        the stop_polling event is set. The polling_ended event is set
        on exit to avoid exceptions on test cleanup.
        """
        while not self._stop_polling.is_set():
            await trio.sleep(0.001)
            data = await trio.to_thread.run_sync(self.jlink.rtt_read, 0, 1024)
            if data:
                await self._send_channel.send(data)

        self._polling_ended.set()

    async def send_all(self, data: bytes, chunk_size: int = 10):
        """
        Send RTT data in smaller chunks with sleeps between writes
        to avoid large single write issues. This avoids RTT dropping
        bytes, reducing RTT test flakiness.
        """
        async with self._write_lock:
            for i in range(0, len(data), chunk_size):
                chunk = data[i:i + chunk_size]
                await trio.to_thread.run_sync(self.jlink.rtt_write, 0, chunk)
                await trio.sleep(0.001)

    async def receive_some(self, max_bytes=None):
        data = await self._recv_channel.receive()
        return bytes(data[:max_bytes]) if max_bytes else bytes(data)
