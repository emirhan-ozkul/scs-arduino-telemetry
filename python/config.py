from dataclasses import dataclass, asdict
from pathlib import Path
import json

@dataclass
class AppConfig:
    udp_host: str = "127.0.0.1"
    udp_port: int = 12345
    serial_port: str = ""
    serial_baud: int = 115200
    serial_reconnect_seconds: float = 3.0
    telemetry_stale_seconds: float = 2.0

    def validate(self) -> None:
        if not self.udp_host:
            raise ValueError("UDP host cannot be empty")
        if not 1 <= self.udp_port <= 65535:
            raise ValueError("UDP port must be between 1 and 65535")
        if not 9600 <= self.serial_baud <= 921600:
            raise ValueError("Serial baud is outside the supported range")

class SettingsStore:
    def __init__(self, path: Path):
        self.path = path

    def load(self) -> AppConfig:
        if not self.path.exists():
            return AppConfig()
        try:
            data = json.loads(self.path.read_text(encoding="utf-8"))
            config = AppConfig(**{k: v for k, v in data.items() if k in AppConfig().__dict__})
            config.validate()
            return config
        except (OSError, json.JSONDecodeError, ValueError, TypeError):
            return AppConfig()

    def save(self, config: AppConfig) -> None:
        config.validate()
        self.path.parent.mkdir(parents=True, exist_ok=True)
        self.path.write_text(json.dumps(asdict(config), indent=2), encoding="utf-8")
