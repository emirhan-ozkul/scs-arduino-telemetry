import socket
import threading
from typing import Callable, Optional

class UDPReceiver:
    def __init__(self, host: str, port: int, on_packet: Callable[[bytes], None], on_error: Callable[[str], None]):
        self.host = host
        self.port = port
        self.on_packet = on_packet
        self.on_error = on_error
        self._stop = threading.Event()
        self._thread: Optional[threading.Thread] = None

    def start(self) -> None:
        if self._thread and self._thread.is_alive():
            return
        self._stop.clear()
        self._thread = threading.Thread(target=self._run, name="udp-receiver", daemon=True)
        self._thread.start()

    def stop(self) -> None:
        self._stop.set()

    def _run(self) -> None:
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
                sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                sock.bind((self.host, self.port))
                sock.settimeout(0.25)
                while not self._stop.is_set():
                    try:
                        packet, _ = sock.recvfrom(65535)
                        self.on_packet(packet)
                    except socket.timeout:
                        continue
                    except OSError as exc:
                        self.on_error(f"UDP receive error: {exc}")
        except OSError as exc:
            self.on_error(f"UDP bind/listen error: {exc}")
