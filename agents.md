# NCSU Rocket Launch — Project Summary

## Overview

This is an Arduino-based flight computer for a model rocket, built with PlatformIO for the **Arduino Nano 33 BLE** board. It uses a **BNO085** 9-DOF IMU (accelerometer, gyroscope, magnetometer, rotation vector) and a **DPS310** barometric pressure/temperature sensor to track the rocket's flight state through its mission profile.

---

## Files

### `platformio.ini`
**Build configuration for PlatformIO.**
- Targets the `nano33ble` board with the Nordic nRF52 platform and Arduino framework.
- Declares dependencies: `Adafruit DPS310`, `Adafruit BNO08x`, `Adafruit BusIO`, and `Adafruit Unified Sensor`.

---

### `README.md`
**Project readme.**
- Minimal description ("Our cool rocket project!!!").
- Links to a shared Google Doc for full documentation.

---

## Source Files (`src/`)

### `src/main.cpp`
**Arduino entry point — `setup()` and `loop()`.**

| Function | Description |
|---|---|
| `setup()` | Initializes serial at 115200 baud, starts SPI (`SPI.begin()`), initializes the BNO085 IMU over SPI (`begin_SPI(BNO08X_CS, BNO08X_INT)`, enabling accelerometer, gyroscope, magnetometer, and rotation vector reports), initializes the DPS310 barometer over SPI (`begin_SPI(DPS310_CS)`, configuring pressure and temperature at 64 Hz with 64x oversampling), and seeds `lastTime`. Hangs (`while(1)`) if either sensor fails to initialize. |
| `loop()` | Main flight loop. Runs a burst of 4 read cycles (3 + 1 extra) — each calls `readBNO08x()`, `readDPS310()`, and `calculateAverages()`. Then calls `figureOutState()` and `writeSD()`. Updates `deltaTime` via `millis()`. Delays 50 ms per iteration (~20 Hz loop rate). |
| `writeSD()` | **Stub / TODO.** Intended to write buffered sensor data to an SD card at 20–50 Hz. Not yet implemented. |

---

### `src/globals.cpp`
**Global variable definitions.**
- Instantiates the `Adafruit_BNO08x` and `Adafruit_DPS310` sensor objects.
- Defines all raw sensor variables, averaged variables, velocity `velocityZ`, timing variables (`currentTime`, `lastTime`, `deltaTime`), and the flight `state` variable.
- The `.hpp` counterpart (`include/globals.hpp`) declares these with `extern` and defines the state constants and pin mappings.

| Variable | Type | Description |
|---|---|---|
| `state` | `int` | Current flight state (STATE_STANDBY, STATE_BOOST, etc.) |
| `bno08x` | `Adafruit_BNO08x` | BNO085 IMU driver instance |
| `sensorValue` | `sh2_SensorValue_t` | Latest sensor event data from the BNO085 |
| `dps` | `Adafruit_DPS310` | DPS310 barometer driver instance |
| `accelX/Y/Z` | `float` | Raw accelerometer readings (m/s²) |
| `gyroX/Y/Z` | `float` | Raw gyroscope readings |
| `magX/Y/Z` | `float` | Raw magnetometer readings |
| `quatReal/I/J/K` | `float` | Raw rotation quaternion components |
| `pressure_hPa` | `float` | Raw barometric pressure (hPa) |
| `temperature_C` | `float` | Raw temperature (°C) |
| `accelX_avg` – `temperature_C_avg` | `float` | Averaged (smoothed) versions of each sensor value |
| `velocityZ` | `float` | Vertical velocity derived from integrated acceleration |
| `currentTime`, `lastTime` | `unsigned long` | Timestamps for delta-time calculation |
| `deltaTime` | `float` | Elapsed time since last loop iteration (ms) |

---

### `src/sensors.cpp`
**Low-level sensor readout functions.**

| Function | Description |
|---|---|
| `readBNO08x()` | Polls the BNO085 for the latest sensor event. On a `wasReset()`, prints a message. On receiving valid data, dispatches by `sensorId`: `SH2_ACCELEROMETER` → `accelX/Y/Z`, `SH2_GYROSCOPE_CALIBRATED` → `gyroX/Y/Z`, `SH2_MAGNETIC_FIELD_CALIBRATED` → `magX/Y/Z`, `SH2_ROTATION_VECTOR` → `quatReal/I/J/K`. |
| `readDPS310()` | Polls the DPS310 for temperature and pressure when both are available. Stores them in `temperature_C` and `pressure_hPa`. |

---

### `src/brains.cpp`
**Flight logic — state estimation and data smoothing.**

| Function | Description |
|---|---|
| `calculateAverages()` | Maintains a rolling 3-sample buffer for every sensor variable. Each buffer has its own index (wraps at 3) and a divisor that grows to 3 (for proper averaging before the buffer is full). For acceleration/gyro/mag/quat values, the function checks `sensorValue.sensorId` to determine which group was just updated. For temperature and pressure, it uses a simple inequality check against the last stored value instead. Updates the corresponding `*_avg` variables. |
| `figureOutState()` | Determines the rocket's flight state by threshold comparisons of `accelZ_avg` and `velocityZ`: |
| | • **STATE_BOOST** — `accelZ_avg >= 20` AND `velocityZ > 1` |
| | • **STATE_COAST** — `accelZ_avg < 1` AND `velocityZ > 0` |
| | • **STATE_APOGEE** — `-1 <= velocityZ <= 1` |
| | • **STATE_DESCENT** — `velocityZ < -1` |
| | • **STATE_LANDED** — `accelZ_avg < 1` AND `velocityZ < 1` |
| | Note: states are **not** mutually exclusive in the current logic (no `else if` chaining), so later conditions can override earlier ones. |
| `calculateVelocityZ()` | Integrates vertical acceleration over time: `velocityZ += accelZ_avg * deltaTime`. Declared but **never called** in `loop()`, so `velocityZ` is never updated at runtime. |

---

## Header Files (`include/`)

### `include/globals.hpp`
**Global declarations, constants, pin mappings, and state enum.**
- Defines SPI pin assignments: `BNO08X_CS = 2`, `BNO08X_INT = 5`, `BNO08X_RESET = 6`, `DPS310_CS = 3`, `SD_CS = 4` (reserved, Nano 33 BLE default SPI: MOSI = 11, MISO = 12, SCK = 13).
- Defines flight state constants: `STATE_STANDBY(0)`, `STATE_BOOST(1)`, `STATE_COAST(2)`, `STATE_APOGEE(3)`, `STATE_DESCENT(4)`, `STATE_LANDED(5)`.
- Declares all global variables with `extern` (mirrors `globals.cpp`).
- Includes `Arduino.h`, `SPI.h`, and both Adafruit sensor libraries.

### `include/brains.hpp`
**Declares `calculateAverages()` and `figureOutState()`.**

### `include/sensors.hpp`
**Declares `readBNO08x()`, `readDPS310()`, and `writeSD()`.**

---

## Known Issues / TODOs

1. **`calculateVelocityZ()` is declared but never called.** Velocity is never integrated, so `figureOutState()` cannot correctly detect transitions (apogee, descent, landed).
2. **State transitions are not exclusive.** The conditions in `figureOutState()` use independent `if` blocks, so multiple states can be set in one loop iteration.
3. **`writeSD()` is a stub.** No data logging to SD card yet.
4. **No initial standby/armed detection.** The first state check immediately jumps to boost if conditions are met.
5. **No abort or safe state handling.** `STATE_STANDBY` is the default but never explicitly set by logic.