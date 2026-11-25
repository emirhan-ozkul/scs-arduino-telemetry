# Native SCS Telemetry Plugin — Build and Installation

## Scope

This document covers only the native telemetry logging plugin under `native_plugin/`. It does not implement UDP, Python, Arduino, or serial communication.

The plugin is a Windows DLL for the SCS Telemetry SDK 1.16 interface.

Runtime compatibility with ETS2 or ATS must be verified by loading and observing the DLL. No ETS2 1.62 compatibility is claimed here.

## Required Tools

- Windows 11
- Visual Studio 2022 Community or another edition with Desktop development with C++
- MSVC x64/x86 build tools
- Windows SDK
- CMake 3.20 or newer

The local environment used for this build had:

- Visual Studio 2022 Community
- MSVC 14.44.35207
- CMake 4.4.4

## Project Structure

```text
native_plugin/
├── CMakeLists.txt
├── exports.def
└── src/
    ├── plugin.cpp
    ├── logger.h
    └── logger.cpp
```

The original SDK remains at:

```text
scs_sdk_1_16/
```

No original SDK file is modified or copied into the plugin source tree.

## SDK and DLL Interface

The plugin accepts telemetry API `SCS_TELEMETRY_VERSION_1_01` and exports:

- `scs_telemetry_init`
- `scs_telemetry_shutdown`

The exports are defined in `native_plugin/exports.def` and linked with `/DEF:` by CMake.

The implementation registers:

| SDK channel | Callback type |
|---|---|
| `truck.speed` | `SCS_VALUE_TYPE_float` |
| `truck.engine.rpm` | `SCS_VALUE_TYPE_float` |
| `truck.engine.gear` | `SCS_VALUE_TYPE_s32` |
| `truck.fuel.amount` | `SCS_VALUE_TYPE_float` |
| `truck.water.temperature` | `SCS_VALUE_TYPE_float` |

## Configure and Build x64

Open **Developer Command Prompt for VS 2022** or **Developer PowerShell for VS 2022**, then run:

```powershell
cd C:\Users\gapcu\Documents\vscode\SDKApp\ETS2sdkapp

& "C:\Program Files\CMake\bin\cmake.exe" `
  -S native_plugin `
  -B build\native_plugin-x64 `
  -G "Visual Studio 17 2022" `
  -A x64

& "C:\Program Files\CMake\bin\cmake.exe" `
  --build build\native_plugin-x64 `
  --config Release
```

The CMake project rejects non-Windows generators and requires MSVC.

Expected DLL output:

```text
build\native_plugin-x64\Release\ETS2TelemetryPlugin.dll
```

CMake may place generated build files in `build/native_plugin-x64` with a configuration subdirectory.

## Build Win32 if Supported

Only use Win32 if the installed game executable is verified to be 32-bit.

Use the x86 Visual Studio generator and Visual Studio's Win32/x86 tools:

```powershell
& "C:\Program Files\CMake\bin\cmake.exe" `
  -S native_plugin `
  -B build\native_plugin-win32 `
  -G "Visual Studio 17 2022" `
  -A Win32

& "C:\Program Files\CMake\bin\cmake.exe" `
  --build build\native_plugin-win32 `
  --config Release
```

Expected Win32 output:

```text
build\native_plugin-win32\Release\ETS2TelemetryPlugin.dll
```

Do not install a Win32 DLL into a 64-bit game or an x64 DLL into a 32-bit game.

## Inspect Exported Functions

From an x64 Developer Command Prompt:

```cmd
dumpbin /exports build\native_plugin-x64\Release\ETS2TelemetryPlugin.dll
```

The relevant export table should include:

```text
scs_telemetry_init
scs_telemetry_shutdown
```

The module definition file requests exactly these exported names. Other symbol names may appear in the DLL but are not part of the required telemetry plugin interface.

Alternative tools:

- `llvm-readobj --coff-exports ETS2TelemetryPlugin.dll`
- A compatible PE export-viewing utility

## Verify Architecture

Before installation, check the ETS2 executable with:

```cmd
dumpbin /headers "<ETS2 executable path>\eurotrucks2.exe"
```

Look for `machine (x64)` or `machine (x86)` in the PE header. Do not assume the installed executable architecture or plugin directory.

Likely modern plugin discovery is:

```text
<ETS2 installation directory>\bin\win_x64\plugins
```

but this must be verified from the actual installation. SDK 1.16 also documents `plugins` under the game binary directory and optional registry plugin discovery.

## Installation

Installation is manual in this phase:

1. Build and inspect the DLL.
2. Verify the ETS2 executable architecture.
3. Verify whether the active ETS2 installation is ETS2 or a compatible SCS game and whether its plugins directory matches the SDK's discovery rules.
4. Copy the matching architecture DLL into the verified plugins directory.
5. Start ETS2 normally.
6. Open the ETS2 console and run `sdk reinit`, if plugin startup has completed and the console command is available.

Do not copy the DLL into the game directory automatically. Do not use the Win32 DLL for a 64-bit executable.

## Log File Location

The plugin writes to:

```text
%TEMP%\ETS2TelemetryPlugin\telemetry_plugin.log
```

or, if `TEMP` is unavailable, `TMP`, followed by a fallback relative location.

Examples:

```text
C:\Users\<User>\AppData\Local\Temp\ETS2TelemetryPlugin\telemetry_plugin.log
```

The plugin appends to the file and flushes each log entry. It logs initialization, shutdown, game identity, channel registration results, state changes, and telemetry values at approximately one-second intervals while the game reports started/driving state.

## Troubleshooting

### DLL is not loaded

Check:

- DLL architecture matches the game executable.
- DLL is in the verified plugin directory or registered through the SDK-documented registry location.
- `scs_telemetry_init` and `scs_telemetry_shutdown` appear in `dumpbin /exports`.
- Windows can locate DLL dependencies. Use `dumpbin /dependents` if loading diagnostics are uncertain.

### Initialization failure

Read the plugin log and ETS2 game log/console for:

- Telemetry API version mismatch.
- Invalid initialization parameters.
- Event registration failure.
- Channel registration failure.
- Unsupported or unknown `game_id`.

The plugin requires telemetry API 1.01 in SDK 1.16. It intentionally does not reject an unknown or newer game major version; it logs a compatibility warning instead.

### Channel registration failure

The log records each channel separately. A result of:

- `SCS_RESULT_not_found`: the game does not provide that channel.
- `SCS_RESULT_unsupported_type`: the game cannot return the requested type.
- `SCS_RESULT_already_registered`: there is a conflicting registration.

Channel callbacks with `SCS_TELEMETRY_CHANNEL_FLAG_no_value` mark values as invalid when `value` is `NULL` or the callback type is unexpected.

### No telemetry values in the log

Verify that:

- The game is in started/driving state rather than a menu or paused state.
- Channel callbacks were registered successfully.
- The plugin log file is created in `%TEMP%\ETS2TelemetryPlugin`.
- The plugin is not being unloaded or reinitialized repeatedly.

## Runtime Testing

The plugin's expected behavior can only be confirmed in the game:

1. Load ETS2 or ATS in the same architecture as the DLL.
2. Start or load a driving session.
3. Observe initialization and channel-registration lines in the plugin log.
4. Drive or change gear and observe approximately once-per-second telemetry lines.
5. Pause in the game menu and verify the plugin logs paused state.
6. Exit the game and verify shutdown is logged.

ETS2 1.62 compatibility is not verified until these runtime observations succeed.
