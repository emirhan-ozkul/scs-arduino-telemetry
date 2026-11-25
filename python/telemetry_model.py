from dataclasses import dataclass, replace
from time import monotonic
from typing import Optional

@dataclass(frozen=True)
class Telemetry:
    protocol: str = "scs-telemetry-v1"
    game_id: Optional[str] = None
    connected: bool = False
    speed_mps: Optional[float] = None
    engine_rpm: Optional[float] = None
    gear: Optional[int] = None
    fuel_l: Optional[float] = None
    fuel_consumption_l_km: Optional[float] = None
    odometer_km: Optional[float] = None
    remaining_distance_m: Optional[float] = None
    water_temperature_c: Optional[float] = None
    received_monotonic: Optional[float] = None

    @property
    def speed_kmh(self) -> Optional[float]:
        return None if self.speed_mps is None else self.speed_mps * 3.6

    def is_fresh(self, stale_seconds: float, now: Optional[float] = None) -> bool:
        if self.received_monotonic is None:
            return False
        current = monotonic() if now is None else now
        return current - self.received_monotonic <= stale_seconds

    def serial_frame(self) -> str:
        def number(value: Optional[float], digits: int = 2) -> str:
            return "NA" if value is None else f"{value:.{digits}f}"

        speed = number(self.speed_kmh, 1)
        rpm = number(self.engine_rpm, 0)
        gear = "NA" if self.gear is None else str(self.gear)
        fuel = number(self.fuel_l, 1)
        remaining = number(None if self.remaining_distance_m is None else self.remaining_distance_m / 1000.0, 2)
        odometer = number(self.odometer_km, 1)
        consumption = number(self.fuel_consumption_l_km, 2)
        temperature = number(self.water_temperature_c, 1)
        return f"T,1,{speed},{rpm},{gear},{fuel},{remaining},{odometer},{consumption},{temperature}\n"

@dataclass
class TelemetryStatus:
    telemetry: Telemetry = Telemetry()
    udp_status: str = "Stopped"
    serial_status: str = "Disconnected"

def decode_packet(raw: bytes) -> Telemetry:
    import json
    try:
        data = json.loads(raw.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise ValueError("Malformed UDP JSON packet") from exc

    if not isinstance(data, dict):
        raise ValueError("UDP packet must be a JSON object")
    if data.get("protocol") != "scs-telemetry-v1":
        raise ValueError("Unsupported telemetry protocol")

    def finite_number(name: str, minimum: Optional[float] = None, maximum: Optional[float] = None) -> Optional[float]:
        value = data.get(name)
        if value is None:
            return None
        if isinstance(value, bool) or not isinstance(value, (int, float)):
            raise ValueError(f"Invalid numeric field: {name}")
        import math
        value = float(value)
        if not math.isfinite(value):
            raise ValueError(f"Non-finite numeric field: {name}")
        if minimum is not None and value < minimum:
            raise ValueError(f"Field below minimum: {name}")
        if maximum is not None and value > maximum:
            raise ValueError(f"Field above maximum: {name}")
        return value

    gear = data.get("gear")
    if gear is not None:
        if isinstance(gear, bool) or not isinstance(gear, int):
            raise ValueError("Invalid gear field")

    game_id = data.get("game_id")
    if game_id is not None and not isinstance(game_id, str):
        raise ValueError("Invalid game_id field")

    connected = data.get("connected")
    if not isinstance(connected, bool):
        raise ValueError("Invalid connected field")

    return Telemetry(
        protocol="scs-telemetry-v1",
        game_id=game_id,
        connected=connected,
        speed_mps=finite_number("speed_mps", -100.0, 100.0),
        engine_rpm=finite_number("engine_rpm", 0.0, 10000.0),
        gear=gear,
        fuel_l=finite_number("fuel_l", 0.0, 10000.0),
        fuel_consumption_l_km=finite_number("fuel_consumption_l_km", 0.0, 100.0),
        odometer_km=finite_number("odometer_km", 0.0, 1e9),
        remaining_distance_m=finite_number("remaining_distance_m", 0.0, 1e9),
        water_temperature_c=finite_number("water_temperature_c", -40.0, 250.0),
        received_monotonic=monotonic(),
    )
