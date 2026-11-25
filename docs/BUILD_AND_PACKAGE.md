# Development, Build and Packaging

## Development

Run Python dependencies:

```powershell
python -m pip install -r python\requirements.txt
```

Run tests:

```powershell
python -m unittest discover -s tests -p "test_*.py" -v
```

Run the GUI:

```powershell
python python\main.py
```

## Native Plugin

Build using the available ScopeCppSDK direct MSVC environment or configure CMake with a complete MSVC/Windows SDK installation.

```powershell
& "C:\Program Files\CMake\bin\cmake.exe" -S native_plugin -B build\native_plugin-x64 -G "Visual Studio 17 2022" -A x64
& "C:\Program Files\CMake\bin\cmake.exe" --build build\native_plugin-x64 --config Release
```

The verified direct-build artifact is:

```text
build\native_plugin-x64\Release\ETS2TelemetryPlugin.dll
```

## Arduino

Compile Uno:

```powershell
& "C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe" compile --fqbn arduino:avr:uno arduino\telemetry_receiver
```

Compile Leonardo:

```powershell
& "C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe" compile --fqbn arduino:avr:leonardo arduino\telemetry_receiver
```

Compile Nano:

```powershell
& "C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe" compile --fqbn arduino:avr:nano arduino\telemetry_receiver
```

## Package

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\package.ps1
```

Outputs:

```text
dist\ETS2Telemetry-Release.zip
```

The portable executable is:

```text
dist\ETS2TelemetryPortable\ETS2Telemetry.exe
```

The native plugin is included for distribution but is not automatically copied into an ETS2/ATS installation.
