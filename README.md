# ETS2 / ATS Telemetry Application

A Windows telemetry bridge for Euro Truck Simulator 2 and American Truck Simulator using the original SCS SDK 1.16 native plugin interface.

## Implemented

- x64 native DLL with SCS telemetry initialization and channel callbacks.
- Localhost UDP JSON telemetry transport.
- Python UDP receiver, validation model and stale-data handling.
- USB serial client with COM reconnection.
- Tkinter Windows GUI.
- Arduino Uno, Leonardo and Nano firmware.
- Unit/integration tests.
- Portable PyInstaller folder and release ZIP.

## Run in Development

```powershell
python -m pip install -r python\requirements.txt
python python\main.py
```

## Package

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\package.ps1
```

Release outputs:

```text
dist\ETS2TelemetryPortable\ETS2Telemetry.exe
dist\ETS2Telemetry-Release.zip
```

## Telemetry

The application exposes speed, engine RPM, gear, fuel, navigation remaining distance, odometer, average fuel consumption and coolant/water temperature.

SDK 1.16 provides `truck.water.temperature`; it is used as the engine temperature value and explicitly mapped in protocol documentation.

## Compatibility

SDK 1.16 declarations extend into ETS2/ATS patch 1.61, but actual game compatibility is not verified. ETS2 1.61 and ATS 1.61 live tests are BLOCKED until those games and their plugin directories are available.

Arduino firmware builds PASS for Uno, Leonardo and Nano. Physical board/display testing is BLOCKED without hardware.

## Documentation

- `docs/PROJECT_AUDIT.md`
- `docs/ARCHITECTURE.md`
- `docs/TELEMETRY_PROTOCOL.md`
- `docs/NATIVE_PLUGIN_BUILD.md`
- `docs/BUILD_AND_PACKAGE.md`
- `docs/TEST_REPORT.md`
- `docs/INSTALLATION.md`
- `docs/ARDUINO_SETUP.md`
