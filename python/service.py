import logging
import threading
import time
from typing import Optional

from config import AppConfig
from serial_client import SerialClient
from telemetry_model import Telemetry, TelemetryStatus, decode_packet
from udp_receiver import UDPReceiver

class TelemetryService:
    def __init__(self, config: AppConfig):
        self.config = config
        self.status = TelemetryStatus()
        self._lock = threading.Lock()
        self._running = False

        self.serial = SerialClient(self._set_serial_status, config.serial_reconnect_seconds)
        self.udp = UDPReceiver(config.udp_host, config.udp_port, self._on_packet, self._set_udp_status)
        self._writer_thread: Optional[threading.Thread] = None

    def start(self) -> None:
        if self._running:
            return
        self._running = True
        self._set_udp_status(f"Listening: {self.config.udp_host}:{self.config.udp_port}")
        self.udp.start()
        self.serial.start(self.config.serial_port, self.config.serial_baud)
        self._writer_thread = threading.Thread(target=self._writer_loop, name="serial-writer", daemon=True)
        self._writer_thread.start()

    def stop(self) -> None:
        self._running = False
        self.udp.stop()
        self.serial.stop()
        self._set_udp_status("Stopped")
        self._set_serial_status("Disconnected")

    def snapshot(self) -> TelemetryStatus:
        with self._lock:
            telemetry = self.status.telemetry
            stale = not telemetry.is_fresh(self.config.telemetry_stale_seconds)
            if stale:
                snapshot = Telemetry(
                    protocol=telemetry.protocol,
                    game_id=telemetry.game_id,
                    connected=False,
                    received_monotonic=telemetry.received_monotonic,
                )
            else:
                snapshot = telemetry
            return TelemetryStatus(snapshot, self.status.udp_status, self.status.serial_status)

    def _on_packet(self, raw: bytes) -> None:
        try:
            telemetry = decode_packet(raw)
            with self._lock:
                self.status.telemetry = telemetry
        except ValueError as exc:
            logging.warning("Rejected telemetry packet: %s", exc)

    def _set_udp_status(self, status: str) -> None:
        with self._lock:
            self.status.udp_status = status

    def _set_serial_status(self, status: str) -> None:
        with self._lock:
            self.status.serial_status = status

    def _writer_loop(self) -> None:
        while self._running:
            with self._lock:
                telemetry = self.status.telemetry

            if telemetry.is_fresh(self.config.telemetry_stale_seconds):
                frame = telemetry.serial_frame()
            else:
                frame = "T,0,NA,NA,NA,NA,NA,NA,NA,NA\n"

            self.serial.write(frame)
            time.sleep(0.25)
