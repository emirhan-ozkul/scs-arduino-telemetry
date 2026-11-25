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

SDK 1.16 declarations extend into ETS2/ATS patch 1.61.

Arduino firmware builds PASS for Uno, Leonardo and Nano.

## Documentation

- `docs/PROJECT_AUDIT.md`
- `docs/ARCHITECTURE.md`
- `docs/TELEMETRY_PROTOCOL.md`
- `docs/NATIVE_PLUGIN_BUILD.md`
- `docs/BUILD_AND_PACKAGE.md`
- `docs/TEST_REPORT.md`
- `docs/INSTALLATION.md`
- `docs/ARDUINO_SETUP.md`

- # 🚚 ETS2 Telemetry v0.1.0
### First Public Preview

**Turn your Euro Truck Simulator 2 experience into a real hardware-based simulation cockpit.**

ETS2 Telemetry is a modular telemetry and hardware integration project designed to connect Euro Truck Simulator 2 with custom physical controls, displays and Arduino-based devices.

The project combines a native SCS telemetry plugin, a Python processing service and Arduino microcontrollers to bring in-game vehicle information to real hardware.

This is the first public preview, featuring a working telemetry pipeline, a Windows desktop application and a physically tested Arduino LCD integration.

---

## ✨ Features

### Real-Time Telemetry
- Live vehicle speed monitoring.
- Engine RPM and gear information.
- Fuel level and average fuel consumption.
- Remaining navigation distance.
- Vehicle odometer.
- Engine coolant temperature.

### Windows Desktop Application
- Graphical application interface.
- Real-time telemetry monitoring.
- UDP-based communication between the native plugin and Python.
- USB Serial communication with Arduino devices.
- Configurable communication settings.

### Arduino Integration
- Arduino Uno support.
- USB Serial telemetry communication.
- Custom telemetry data parsing.
- 1602A LCD Keypad Shield integration.
- Global telemetry variables available for custom hardware development.

### Modular Architecture
The project is designed to support additional hardware without requiring changes to the core telemetry processing system.

Planned extensions include:
- Multiple Arduino controllers.
- 7-segment gear displays.
- USB HID Button Boxes.
- Arduino Leonardo keyboard and gamepad emulation.
- RFID-based ignition controls.
- Custom physical cockpit components.

---

## ⚙️ System Architecture

```text
Euro Truck Simulator 2
          |
          v
  SCS Telemetry Plugin
          |
          v
    UDP Communication
          |
          v
  Python Telemetry Service
          |
          +------------------+
          |                  |
          v                  v
    Windows GUI         USB Serial
                             |
                             v
                        Arduino Uno
                             |
                             v
                       Physical LCD
```

The architecture is modular, allowing telemetry processing and hardware control to evolve independently.

---

## 🧪 Tested Configuration

| Component | Status |
|---|---|
| Operating System | Windows 10 / 11 |
| Target Game | ETS2 1.61 |
| Native Plugin | Windows x64 |
| Python Telemetry Service | Tested |
| UDP Communication | Tested |
| Arduino Uno | Tested |
| 1602A LCD Keypad Shield | Physically tested |
| USB Serial Communication | Physically tested |

Telemetry data has been verified during an actual ETS2 session and successfully displayed on the Arduino LCD.

ATS runtime compatibility and additional hardware configurations still require separate testing.

---

## 📦 Installation

1. Download the Windows x64 portable package from the Assets section below.
2. Extract the complete ZIP archive.
3. Follow the installation instructions included in the package or repository.
4. Install the native telemetry plugin into the appropriate ETS2 plugin directory.
5. Connect your Arduino device via USB.
6. Upload the Arduino telemetry receiver firmware if necessary.
7. Launch ETS2 Telemetry.
8. Select your Arduino COM port and press **Start**.

**Note:** The native plugin must match the architecture of your game executable. Do not copy the DLL into the game directory without checking its installation requirements.

For detailed instructions, see the repository documentation.

---

## 🚧 Development Status

This release represents the first working version of the telemetry platform.

The following features are planned for future releases:

- Arduino Leonardo USB HID Button Box.
- Multi-Arduino communication.
- Dedicated 7-segment gear display.
- RFID-based ignition simulation.
- Additional physical dashboard components.
- Expanded hardware configuration options.
- Improved installation and configuration experience.

---

## ⚠️ Known Limitations

- ATS 1.61 runtime compatibility has not yet been independently verified.
- Additional Arduino hardware configurations are still under development.
- HID Button Box and RFID functionality are not included in this release.
- Physical LCD and other display configurations may require individual firmware customization.

---

## 🤝 Feedback and Contributions

Contributions, bug reports and feature suggestions are welcome.

If you encounter an issue, please open a GitHub Issue and include:

- Game version.
- Operating system.
- Arduino board model.
- Relevant application or plugin logs.
- Steps to reproduce the problem.

Your feedback will help improve the project and expand hardware compatibility.

---

**Thank you for checking out ETS2 Telemetry!**

*Built for simulation enthusiasts who want to bring their virtual trucks into the physical world.*
