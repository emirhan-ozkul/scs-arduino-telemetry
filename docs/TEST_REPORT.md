# Test Report

## Status Codes

- PASS: Tested successfully.
- FAIL: Executed and failed.
- BLOCKED: External game, board, or hardware was unavailable.

## Automated Python Tests

| Test | Status | Evidence |
|---|---|---|
| Telemetry decoding and m/s-to-km/h conversion | PASS | 9 unittest tests passed |
| Missing/null values remain unavailable | PASS | unittest passed |
| Invalid protocol/numeric values rejected | PASS | unittest passed |
| Serial fixed-field frame | PASS | unittest passed |
| Stale telemetry detection | PASS | unittest passed |
| Localhost UDP receive | PASS | unittest passed |

## Native Plugin

| Test | Status | Evidence |
|---|---|---|
| x64 PE architecture | PASS | `dumpbin /headers` reported machine x64 |
| Required exports | PASS | `dumpbin /exports` reported both required exports |
| Direct source compilation/link | PASS | x64 DLL generated |
| UDP implementation build | PASS | updated DLL compiled and linked |
| ETS2 1.61 loading | BLOCKED | ETS2 executable/game session unavailable |
| ATS 1.61 loading | BLOCKED | ATS executable/game session unavailable |

## Arduino Firmware

| Board | Status | Evidence |
|---|---|---|
| Arduino Uno | PASS | 4898 bytes flash, 356 bytes RAM |
| Arduino Leonardo | PASS | 6930 bytes flash, 321 bytes RAM |
| Arduino Nano | PASS | 4898 bytes flash, 356 bytes RAM |
| Python-to-firmware protocol test | PASS | 13 Python tests passed, including live, NA and disconnected frames |
| Physical serial/display test | BLOCKED | Arduino hardware unavailable |

## GUI and Packaging

| Test | Status | Evidence |
|---|---|---|
| Development GUI startup | PASS | Tkinter window opened and was responding |
| Portable GUI startup | PASS | `ETS2Telemetry.exe` remained alive as `ETS2Telemetry`, title and responding verified |
| Portable folder | PASS | `dist/ETS2TelemetryPortable` generated |
| Release ZIP | PASS | `dist/ETS2Telemetry-Release.zip` generated |

## Known External Blockers

- ETS2 1.61 actual game test.
- ATS 1.61 actual game test.
- Serial COM port hardware test.
- Physical Arduino display test.
