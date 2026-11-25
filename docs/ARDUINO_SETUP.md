# Arduino Telemetry Setup

## Scope

The Arduino firmware under `arduino/telemetry_receiver/` only receives and validates telemetry packets from the Python application. It has no LCD, 7-segment, LED or button code. Hardware-specific code is added by the user in `setupUserHardware()` and `updateUserHardware()`.

## Serial Connection

- Transport: USB Serial
- Baud: `115200`
- Frame terminator: newline `\n`
- Packet field count: exactly 10

## Protocol

Live:

```text
T,1,speed_kmh,rpm,gear,fuel_l,remaining_km,odometer_km,consumption_l_km,temperature_c
```

Example:

```text
T,1,72.0,1450,8,120.5,25.00,12345.0,0.28,88.0
```

Disconnected/stale:

```text
T,0,NA,NA,NA,NA,NA,NA,NA,NA
```

Python sends this protocol unchanged. The Arduino does not convert m/s to km/h.

## Global Variables

### Vehicle

- `float speedKmh`
- `int32_t gear`
- `bool hasSpeed`
- `bool hasGear`

### Engine

- `float engineRpm`
- `float waterTemperatureC`
- `bool hasEngineRpm`
- `bool hasWaterTemperature`

### Fuel

- `float fuelLiters`
- `float fuelConsumptionLPerKm`
- `bool hasFuel`
- `bool hasFuelConsumption`

### Navigation

- `float remainingKm`
- `float odometerKm`
- `bool hasRemainingDistance`
- `bool hasOdometer`

### Connection

- `bool telemetryConnected`
- `unsigned long lastTelemetryMs`
- `uint32_t invalidFrameCount`

## Availability Flags

Use both connection and field availability:

```cpp
if (telemetryConnected && hasSpeed)
{
    // Use speedKmh.
}

if (telemetryConnected && hasGear)
{
    // Use gear.
}
```

When a field is `NA`, its numeric global value is preserved and its `has...` flag is set to `false`.

## T,0 and Timeout Behavior

A `T,0,...` packet sets `telemetryConnected = false`, clears all `has...` flags and preserves numeric global values.

If no live/disconnected packet is received for 2500 ms, `updateConnectionStatus()` sets `telemetryConnected = false` and clears availability flags. Numeric values remain unchanged.

## Where to Add Hardware Code

### setupUserHardware()

Called once from `setup()`.

Add only user hardware initialization here, for example pin setup or board initialization. Do not add display or serial protocol code here.

### updateUserHardware()

Called every loop iteration.

Add LCD, 7-segment, LED, button or future hardware code here.

```cpp
void updateUserHardware()
{
    if (telemetryConnected && hasSpeed)
    {
        // Update the user's speed hardware.
    }
}
```

## Arduino IDE Upload

1. Open `arduino/telemetry_receiver/telemetry_receiver.ino`.
2. Select `Arduino Uno` under Tools > Board.
3. Select the USB COM port under Tools > Port.
4. Click Verify.
5. Click Upload.

Keep USB Serial baud at `115200`; debug `Serial.print()` calls must not be added because they would corrupt the telemetry packet stream.

## Validation Behavior

The firmware rejects:

- A field count other than 10.
- A packet type other than `T`.
- A frame type other than `0` or `1`.
- Malformed numeric text.
- Out-of-range numeric values.
- NaN/Infinity-like values.
- A `T,1` frame with invalid numeric fields.
- A `T,0` frame containing a value other than `NA`.

Invalid frames increment `invalidFrameCount` and do not commit partial global values.
