import threading
import time
from typing import Callable, Optional
import serial

class SerialClient:
    def __init__(self, on_status: Callable[[str], None], reconnect_seconds: float = 3.0):
        self.on_status = on_status
        self.reconnect_seconds = reconnect_seconds
        self.port: Optional[str] = None
        self.baud = 115200
        self._stop = threading.Event()
        self._thread: Optional[threading.Thread] = None
        self._connection: Optional[serial.Serial] = None

    def start(self, port: str, baud: int) -> None:
        self.port = port
        self.baud = baud
        if not port:
            self.on_status("Disconnected: no COM port selected")
            return
        self._stop.clear()
        self._thread = threading.Thread(target=self._run, name="serial-client", daemon=True)
        self._thread.start()

    def stop(self) -> None:
        self._stop.set()
        if self._connection:
            try:
                self._connection.close()
            except Exception:
                pass

    def write(self, frame: str) -> bool:
        connection = self._connection
        if connection is None or not connection.is_open:
            return False
        try:
            connection.write(frame.encode("ascii"))
            connection.flush()
            return True
        except (serial.SerialException, OSError) as exc:
            self.on_status(f"Disconnected: {exc}")
            try:
                connection.close()
            except Exception:
                pass
            self._connection = None
            return False

    def _run(self) -> None:
        while not self._stop.is_set():
            try:
                connection = serial.Serial(self.port, self.baud, timeout=0.2)
                self._connection = connection
                self.on_status(f"Connected: {self.port} @ {self.baud}")
                while not self._stop.is_set() and connection.is_open:
                    time.sleep(0.1)
            except (serial.SerialException, OSError) as exc:
                self._connection = None
                self.on_status(f"Disconnected: {exc}")

            if not self._stop.is_set():
                time.sleep(self.reconnect_seconds)
