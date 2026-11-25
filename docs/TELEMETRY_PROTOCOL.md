# Telemetry Protocol

## Plugin to Python: UDP JSON

Transport:

- IPv4 loopback `127.0.0.1`
- UDP port `12345` by default
- Native environment override: `ETS2_TELEMETRY_UDP_PORT`
- JSON packet version: `scs-telemetry-v1`
- Target send interval: 10 Hz

Packet:

```json
{
  "protocol": "scs-telemetry-v1",
  "game_id": "eut2",
  "connected": true,
  "speed_mps": 20.0,
  "engine_rpm": 1450,
  "gear": 8,
  "fuel_l": 120.5,
  "fuel_consumption_l_km": 0.28,
  "odometer_km": 12345,
  "remaining_distance_m": 25000,
  "water_temperature_c": 88
}
```

Unavailable values are `null`. Invalid JSON, unsupported protocol versions, wrong types and out-of-range values are rejected in Python.

## Python to Arduino: CSV Serial

Serial settings:

- 115200 baud
- newline terminator `\n`
- fixed field count: 10

Fresh frame:

```text
T,1,speed_kmh,rpm,gear,fuel_l,remaining_km,odometer_km,consumption_l_km,temperature_c
```

Example:

```text
T,1,72.0,1450,8,120.5,25.00,12345.0,0.28,88.0
```

Stale frame:

```text
T,0,NA,NA,NA,NA,NA,NA,NA,NA
```

Unavailable fields use `NA`. Arduino validates the `T` packet type, frame type, field count and numeric syntax. It does not allocate dynamic strings for packet parsing.
