# Telemetry Application Architecture

## Logical Flow

```text
ETS2 / ATS
  -> SCS 1.16 native DLL callbacks
  -> non-blocking localhost UDP JSON (scs-telemetry-v1)
  -> Python UDP receiver
  -> Telemetry model and validation
  -> USB serial client
  -> Arduino Uno/Leonardo/Nano
  -> physical displays/LEDs

Tkinter GUI
  -> service start/stop/configuration/status
  -> no telemetry processing in GUI components
```

## Components

- `native_plugin/`: C++ DLL. Registers SDK channels and sends fixed packets every 100 ms when the game callback runs.
- `python/udp_receiver.py`: UDP socket thread.
- `python/telemetry_model.py`: decoding, validation, unit conversion and serial framing.
- `python/serial_client.py`: COM reconnection and serial writes on a worker thread.
- `python/service.py`: service lifecycle and stale telemetry management.
- `python/app.py`: Tkinter GUI.
- `arduino/telemetry_receiver/`: AVR serial parser and timeout behavior.

## Units

- Plugin speed: metres per second.
- Python GUI speed: kilometres per hour.
- Fuel: litres.
- Remaining distance: kilometres on serial/GUI; metres inside plugin packet.
- Odometer: kilometres.
- Fuel consumption: litres per kilometre in this SDK value.
- Water temperature: degrees Celsius.

No invalid or unavailable value is silently converted to zero. Unavailable fields are `null` in native JSON and `NA` in serial data.

## Threading

Native SDK callbacks remain short and do not wait for Python or serial. Python UDP and serial operations run in daemon threads. Tkinter refreshes status every 250 ms; no socket or COM operation is placed directly in the GUI loop.

## Stale Data

The Python model is stale after 2 seconds by default. While stale, Python sends `T,0,...` to Arduino. Arduino also applies a 2.5-second stale timeout after its last valid frame.
