# SCS SDK 1.16 Analysis

## Scope

This document describes only what is declared or demonstrated by the original SDK files in `scs_sdk_1_16/` in this workspace.

The document does **not** establish runtime compatibility with any installed ETS2 build, including ETS2 1.62. Compatibility with an installed game must be verified after installing and loading the plugin.

## SDK Structure

Relevant SDK files:

- `readme.txt`: plugin discovery, calling conventions, threading, versioning, and API overview.
- `include/scssdk.h`: shared SDK types, calling convention macros, version macros, result codes, and initialization parameters.
- `include/scssdk_telemetry.h`: telemetry API versions, `scs_telemetry_init`, `scs_telemetry_shutdown`, and initialization parameters.
- `include/scssdk_telemetry_event.h`: frame, pause/start, configuration, and gameplay events.
- `include/scssdk_telemetry_channel.h`: channel callbacks, registration, and channel flags.
- `include/scssdk_value.h`: callback value unions and data types.
- `include/common/scssdk_telemetry_truck_common_channels.h`: truck channels.
- `include/eurotrucks2/scssdk_eut2.h`: ETS2 game ID and ETS2 telemetry game-version constants.
- `include/eurotrucks2/scssdk_telemetry_eut2.h`: ETS2-supported-game context and channel/configuration limitations.
- `examples/telemetry/telemetry.cpp`: complete initialization, event, channel, callback, and shutdown example.
- `examples/telemetry/telemetry.def`: Windows DLL exports.
- `examples/telemetry/telemetry.vcproj`: Visual C++ Win32/x64 DLL configurations.
- `examples/telemetry/Makefile`: Linux shared-library build example.

## Required Native Plugin Interface

The native library is dynamically loaded by the game.

Required telemetry exports demonstrated by the SDK:

```cpp
SCSAPI_RESULT scs_telemetry_init(
    const scs_u32_t version,
    const scs_telemetry_init_params_t *const params);

SCSAPI_VOID scs_telemetry_shutdown(void);
```

The Windows export definition must expose both names. SDK `telemetry.def` contains:

```text
EXPORTS
scs_telemetry_init=scs_telemetry_init
scs_telemetry_shutdown=scs_telemetry_shutdown
```

The DLL must use architecture matching the game executable. The SDK defines `__stdcall` and 32/64-bit architecture handling for Microsoft compilers. The sample project contains Win32 and x64 configurations.

## Initialization Sequence

Based on `examples/telemetry/telemetry.cpp`:

1. Accept only telemetry API version `SCS_TELEMETRY_VERSION_1_01` in this SDK example.
2. Receive `scs_telemetry_init_params_v101_t`; this is an alias of the v1.00 parameter layout.
3. Inspect `common.game_id` and `common.game_version` where needed.
4. Register required frame and pause/start event callbacks.
5. Optionally register configuration and gameplay event callbacks.
6. Register channel callbacks for selected channels.
7. Initialize state values and return `SCS_RESULT_ok`.

The engine starts telemetry in a paused state. `scs_telemetry_init` may be followed by pause/start events.

`scs_telemetry_shutdown` must be safe and prepare state for possible repeated init/shutdown cycles. The SDK says registrations are removed automatically at shutdown, but resources owned by the plugin must still be cleaned up explicitly.

## Event Callbacks

The relevant event declarations are:

- `SCS_TELEMETRY_EVENT_frame_start`: received before channel callbacks for a frame.
- `SCS_TELEMETRY_EVENT_frame_end`: received after channel callbacks for a frame.
- `SCS_TELEMETRY_EVENT_paused`: game entered paused state.
- `SCS_TELEMETRY_EVENT_started`: player is now driving.
- `SCS_TELEMETRY_EVENT_configuration`: static configuration changed.
- `SCS_TELEMETRY_EVENT_gameplay`: gameplay event occurred.

Frame-start timing fields are `render_time`, `simulation_time`, and `paused_simulation_time`, all measured in microseconds.

## Telemetry Channels Used by the MVP

All channels below are declared in `include/common/scssdk_telemetry_truck_common_channels.h`.

| Plugin packet field | SDK channel | SDK callback type | Unit/meaning |
|---|---|---|---|
| `speed_kmh` in serial; native `speed` from SDK | `truck.speed` | `SCS_VALUE_TYPE_float` | Speedometer speed in meters per second; negative values represent reverse |
| `engine_rpm` | `truck.engine.rpm` | `SCS_VALUE_TYPE_float` | Engine RPM |
| `gear` | `truck.engine.gear` | `SCS_VALUE_TYPE_s32` | Engine-selected gear; positive forward, zero neutral, negative reverse |
| `fuel` | `truck.fuel.amount` | `SCS_VALUE_TYPE_float` | Fuel amount in litres |
| `water_temperature` | `truck.water.temperature` | `SCS_VALUE_TYPE_float` | Degrees Celsius |

The channel registration type must match the supported callback type. The SDK permits float channels to also provide doubles, but the MVP should request `SCS_VALUE_TYPE_float` because the declarations identify float as the channel type. Unavailability handling is separate: without `SCS_TELEMETRY_CHANNEL_FLAG_no_value`, callbacks are only delivered for available values; with it, callbacks receive `NULL` when unavailable.

## Available Channel Callback Data Types

Channel callbacks receive `scs_value_t *` from `scssdk_value.h`.

Relevant union members:

- `value_float.value`: `scs_float_t`, 32-bit float.
- `value_s32.value`: `scs_s32_t`, signed 32-bit integer.

The generic value structure also declares boolean, unsigned/signed integers, double, vectors, placements, Euler, and string values. Only types required by registered channels should be asserted or parsed.

## Unit Conversion

The SDK explicitly declares `truck.speed` in meters per second. The Python application must convert it to kilometres per hour as:

```text
speed_kmh = speed_mps * 3.6
```

The remaining initial fields do not require conversion according to the SDK declarations:

- RPM: raw SDK RPM value.
- Gear: signed integer.
- Fuel: litres.
- Water temperature: degrees Celsius.

## Channel Registration and Callback Behavior

Channel registration signature:

```cpp
register_for_channel(
    channel_name,
    SCS_U32_NIL,
    value_type,
    flags,
    callback,
    context);
```

Use `SCS_U32_NIL` for these normal, non-indexed channels. `SCS_TELEMETRY_CHANNEL_FLAG_none` is suitable for basic float/s32 callbacks. `SCS_TELEMETRY_CHANNEL_FLAG_no_value` can make availability explicit, but each callback must validate `value != NULL` and value type.

Channel and event callbacks run on the game's main thread. The SDK is not thread-safe, and calls back to the game must happen only from permitted SDK callbacks/initialization. Network sends must be short and non-blocking so they do not block the game thread.

## Local UDP Design Consequence

SDK 1.16 provides no built-in network/IPC transport. The native plugin must use local OS APIs itself.

The MVP should:

- Send from the `frame_end` callback or a lower-frequency channel callback.
- Target `127.0.0.1:<configured UDP port>`.
- Use a non-blocking UDP socket on Windows.
- Keep JSON or binary packet formatting outside the SDK declarations; there is no SDK packet format.

JSON is practical for the Python MVP. Packet field names are application-defined and must not be mistaken for SDK channel names.

## Build Requirements

The original SDK examples support:

- Windows Microsoft Visual C++ through `telemetry.vcproj`.
- Linux GCC through the example `Makefile` and `-fPIC --shared`.

The supplied Windows project is a Visual Studio 2008-style `.vcproj` and includes both Win32 and x64 DLL configurations. Visual Studio 2008 is the format demonstrated by the SDK, but this workspace may use a newer Visual Studio toolchain. Building the plugin in Modern Visual Studio will require project modernization or an equivalent CMake/MSBuild project.

For the implementation, use a Windows C/C++ toolchain that:

- Produces a Windows DLL.
- Defines `_MSC_VER` for the SDK Microsoft compiler branch.
- Uses matching CPU architecture (32-bit or 64-bit).
- Adds `scs_sdk_1_16/include` and all SDK subdirectories to include paths, as needed.
- Supplies `native_plugin/src/telemetry.def` as the linker module-definition file if building with MSVC.
- Exports exactly `scs_telemetry_init` and `scs_telemetry_shutdown`.

The SDK example uses `__stdcall` for Microsoft exports. Architecture and calling convention must match the game executable.

No successful build has been performed in this phase. The final build instructions must distinguish SDK sample support from verified local build results.

## Windows Plugin Installation

The SDK states that plugins are discovered by:

1. Libraries in the game's `plugins` subdirectory beneath the game binary directory, for example `/bin/win_x86/plugins`.
2. Windows registry values under:
   - `HKEY_LOCAL_MACHINE\SOFTWARE\SCS Software\Euro Truck Simulator 2\Plugins`
   - For 32-bit game targeting on 64-bit Windows: `HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\SCS Software\Euro Truck Simulator 2\Plugins`

These registry paths are documented by SDK 1.16. An actual ETS2 installation path and actual executable architecture were not part of the SDK files and cannot be assumed here.

Do not copy the DLL until the target executable architecture and active installation directory have been verified. A likely modern layout would place a 64-bit DLL under `bin/win_x64/plugins`, but this is not verified here and must not be treated as an SDK guarantee.

## ETS2 Game-Version Information

SDK `scssdk_eut2.h` defines `SCS_GAME_ID_EUT2` as `"eut2"`.

`scssdk_telemetry_eut2.h` defines ETS2 telemetry game-version `1.21` as the SDK's current ETS2 game-version constant:

```cpp
SCS_TELEMETRY_EUT2_GAME_VERSION_CURRENT
SCS_TELEMETRY_EUT2_GAME_VERSION_1_21
```

Its change history maps these constants to patches through 1.61, not a current-world guarantee for a later ETS2 release:

- SDK source comments describe `1.21` as patch 1.61 and `1.19`/`1.20` as patches 1.60/1.61.
- The SDK does not contain an ETS2 1.62 game-version declaration or compatibility evidence.
- ETS2 1.62 itself may identify a different `game_version` to telemetry plugins.

The sample plugin logs a warning if the game major version is newer than the SDK's implemented/current ETS2 major version. It does not prove that the channel interpretation remains safe.

## Known SDK Limitations

- SDK 1.16 documents telemetry only; no input API implementation is needed for this MVP.
- The SDK may omit a channel when a game no longer supports it or does not support the requested type.
- Channel registration can fail with `SCS_RESULT_not_found` or `SCS_RESULT_unsupported_type`.
- The SDK has no UDP or JSON transport.
- Configuration and gameplay events are optional MVP information.
- Channel callback order is undefined.
- Fuel/water-speed and all displayed values are as represented by the SDK; the SDK notes approximate temperatures/pressures in some cases.
- The supplied sample writes a log file only and has no Python/Arduino transport.

## Compatibility Risks

1. Major API compatibility: telemetry API initialization only succeeds for a negotiated API version accepted by the plugin. SDK 1.16 implementation accepts only 1.01 in the supplied example.
2. Game major telemetry compatibility: newer game major versions may change channel interpretation. The SDK example explicitly warns for major-version mismatch.
3. Architecture mismatch: loading a Win32 plugin into a 64-bit ETS2 executable or vice versa will not work.
4. Missing channel: a channel may fail registration in newer games.
5. Channel changes: names and supported types must remain those declared by the SDK; additional or removed channels may not be discoverable dynamically in this MVP.
6. Plugin discovery: a Steam, Microsoft Store, or custom ETS2 installation may have a different executable location than the SDK examples assume.
7. Security/registry: registry plugin discovery requires appropriate permissions and may require elevation during installation.

## Verification Still Required

- Actual ETS2 executable location.
- ETS2 executable architecture (win_x86 or win_x64).
- ETS2 build/game telemetry `game_version` observed during `scs_telemetry_init`.
- Whether this SDK 1.16 DLL loads successfully in the installed game.
- Registration result for speed, RPM, gear, fuel, and water temperature channels.
- Real callback values and availability while paused and while driving.

Until these checks are performed, claim only: “the plugin is intended for this SDK 1.16 interface,” not “this plugin is compatible with ETS2 1.62.”
