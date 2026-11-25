# Project Audit

## Audit Scope

Audited repository: `ETS2sdkapp`

No files were deleted or overwritten during the audit. The repository had no commits at audit time; existing changes under the workspace were treated as user content.

## Current Architecture

The repository currently contains:

```text
ETS2/ATS game
  -> SCS SDK 1.16 native DLL
  -> file log (current implementation)

Not yet implemented:
  -> UDP to Python
  -> Python service/model/GUI
  -> USB serial to Arduino
  -> Arduino firmware
  -> release package
```

## Existing Components
n
### Native plugin

- `native_plugin/CMakeLists.txt`: CMake Windows/MSVC DLL project.
- `native_plugin/exports.def`: exports `scs_telemetry_init` and `scs_telemetry_shutdown`.
- `native_plugin/src/plugin.cpp`: SDK 1.01 initialization, five channel registrations, pause/start and frame callbacks, compatibility logging.
- `native_plugin/src/logger.cpp/.h`: timestamped file logger in `%TEMP%\ETS2TelemetryPlugin`.
- The plugin has a verified x64 build artifact at `build/native_plugin-x64/Release/ETS2TelemetryPlugin.dll`.
- `dumpbin` verified the PE architecture as x64 and the two required exports.

Registered native channels currently:

- `truck.speed`
- `truck.engine.rpm`
- `truck.engine.gear`
- `truck.fuel.amount`
- `truck.water.temperature`

The implementation does not yet send UDP packets.

### SDK

- Original SDK 1.16 files remain under `scs_sdk_1_16/` and were not modified.
- `scs_sdk_1_16/include/eurotrucks2/scssdk_telemetry_eut2.h` declares ETS2 telemetry game-version constants through 1.21 for patch 1.61.
- `scs_sdk_1_16/include/amtrucks/scssdk_telemetry_ats.h` declares ATS telemetry game-version constants through 1.08 for patch 1.61.
- SDK 1.16 declarations do not prove runtime compatibility with actual ETS2/ATS 1.61 installations.

### Documentation

- `docs/SDK_ANALYSIS.md`: SDK structure and interface analysis.
- `docs/NATIVE_PLUGIN_BUILD.md`: native build and installation instructions.

### Missing Components

- UDP sender in native plugin.
- Remaining-distance, odometer and fuel-consumption SDK channel registrations.
- Python receiver and normalized model.
- USB serial client and reconnection behavior.
- Tkinter GUI.
- Arduino firmware and board builds.
- Unit and integration tests.
- PyInstaller portable package.
- Packaging script and release output.
- Architecture, protocol, test, build-package, README and changelog documentation.

## SDK Channel Findings

Actual SDK declarations include:

| Value | Channel | Unit/type |
|---|---|---|
| Vehicle speed | `truck.speed` | float, m/s |
| Engine RPM | `truck.engine.rpm` | float, RPM |
| Gear | `truck.engine.gear` | s32, positive forward/0 neutral/negative reverse |
| Fuel amount | `truck.fuel.amount` | float, litres |
| Fuel consumption | `truck.fuel.consumption.average` | float, litres/km |
| Odometer | `truck.odometer` | float, km |
| Remaining navigation distance | `truck.navigation.distance` | float, metres |
| Engine coolant temperature | `truck.water.temperature` | float, degrees Celsius |

No separate “engine temperature” channel was found. The implementation will expose `truck.water.temperature` as coolant/engine temperature and document that name mapping.

## Compatibility Findings

- Native DLL build is verified for x64 and required exports are verified.
- Runtime ETS2/ATS 1.61 loading is not verified.
- Plugin architecture and actual game executable architecture must be compared manually.
- SDK 1.16 source declares 1.61-compatible game telemetry-version constants, but this is declaration evidence, not actual runtime evidence.
- ETS2 1.61 live test: BLOCKED until an ETS2 installation and the plugin can be run.
- ATS 1.61 live test: BLOCKED until an ATS installation and the plugin can be run.

## Dependencies and Build Requirements

- Windows 11
- Visual Studio 2022 C++ build tools with Windows SDK for native DLL builds
- CMake 3.20+
- Python 3.11+
- `pyserial`
- Tkinter, included with standard Windows Python installations
- PyInstaller for distributable packaging
- Arduino CLI for firmware compilation
- Uno/Nano/Leonardo board cores and matching compiler packages

## Recommended Development Sequence

1. Extend native plugin with non-blocking localhost UDP packets and additional SDK channels.
2. Implement and test a Python UDP receiver and telemetry model.
3. Implement serial communication and reconnection.
4. Implement a Tkinter GUI around the service API.
5. Implement Arduino serial framing and stale telemetry behavior.
6. Add simulated UDP, serial, model and firmware/protocol tests.
7. Package the application and native DLL with PyInstaller.
8. Record live-game and hardware tests separately as PASS, FAIL or BLOCKED.
