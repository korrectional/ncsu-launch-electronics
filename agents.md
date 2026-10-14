# NCSU Rocket Launch — Project Summary

## Overview

This is an Arduino-based flight computer for a model rocket, built with PlatformIO for the **Arduino Nano Every** board. It uses a **DPS310** barometric pressure/temperature sensor as its only sensor, and approximates the rocket's height from the pressure reading to track the flight state.

> **Note:** The **BNO085** 9-DOF IMU was previously part of this design. It has been dropped; all BNO085 code is commented out (not deleted) so it can be restored if needed. The BNO085 library dependency is also commented out of `platformio.ini`.

---

## Files

### `platformio.ini`
**Build configuration for PlatformIO.**
- Targets the `nano_every` board (ATmega4809) with the `atmelmegaavr` platform and Arduino framework.
- Declares dependencies: `Adafruit DPS310`, `Adafruit BusIO`, and `Adafruit Unified Sensor`. The `Adafruit BNO08x` entry is commented out.
- Extra env `nano_every_diag` builds the raw-SPI diagnostic console (`src/diag.cpp`) instead of `main.cpp`.
- Legacy `nano33ble` env kept for the original hardware.

---

### `README.md`
**Project readme.**
- Minimal description ("Our cool rocket project!!!").
- Links to a shared Google Doc for full documentation.
- Contains the SPI wiring tables. **These still document the BNO085** (chip select, INT, RST, and the PS0/PS1 SPI-mode jumpers) even though the sensor is no longer used.

---

## Source Files (`src/`)

### `src/main.cpp`
**Arduino entry point — `setup()` and `loop()`.**

| Function | Description |
|---|---|
| `setup()` | Initializes serial at 115200 baud, parks the chip selects high, starts SPI (`SPI.begin()`), runs `debugSPI()`, then calls `initSensors()`. Seeds `lastTime` if the DPS310 came up. |
| `initSensors()` | Calls `tryInitDPS()` only. Returns true when the DPS310 is up. |
| `tryInitDPS()` | Initializes the DPS310 over SPI (`begin_SPI(DPS310_CS)`), configuring pressure and temperature at 64 Hz with 64x oversampling. On failure it dumps the raw ID register (`0x0D`) in SPI modes 0 and 3. |
| `debugSPI()` | Prints the MISO idle level and parks the chip selects. |
| `loop()` | Retries sensor init every 2 s until the DPS310 is up. Then computes `deltaTime`, runs 4 `readDPS310()` cycles, calls `calculateVelocityZ()`, `figureOutState()`, and `writeSD()`, prints DPS + altitude readings ~2x/sec, and delays 50 ms (~20 Hz). |
| `writeSD()` | **Stub / TODO.** Intended to write buffered sensor data to an SD card at 20–50 Hz. Not yet implemented. |
| `tryInitBNO()`, `debugBNO()` | **Commented out** along with the BNO085. |

---

### `src/globals.cpp`
**Global variable definitions.**
- Instantiates the `Adafruit_DPS310` sensor object.
- Defines raw variables (`pressure_hPa`, `temperature_C`), averaged variables (`pressure_hPa_avg`, `temperature_C_avg`), the barometric height variables, velocity `velocityZ`, timing variables (`currentTime`, `lastTime`, `deltaTime`), and the flight `state` variable.
- BNO085 variables (`accelX/Y/Z`, `gyroX/Y/Z`, `magX/Y/Z`, `quatReal/I/J/K`, their `_avg` versions, and their rolling buffers) are **commented out**.

| Variable | Type | Description |
|---|---|---|
| `state` | `int` | Current flight state (STATE_STANDBY, STATE_BOOST, etc.) |
| `dps` | `Adafruit_DPS310` | DPS310 barometer driver instance |
| `pressure_hPa` | `float` | Raw barometric pressure (hPa) |
| `temperature_C` | `float` | Raw temperature (°C) |
| `pressure_hPa_avg`, `temperature_C_avg` | `float` | Averaged (smoothed) versions of the DPS readings |
| `groundPressure_hPa` | `float` | Reference pressure captured on the pad; `0` until captured |
| `altitude_m` | `float` | Barometric height above the pad (m), from the raw pressure |
| `altitude_m_avg` | `float` | Smoothed height, used by the state logic |
| `maxAltitude_m` | `float` | Highest height seen this flight |
| `lastAltitude_m` | `float` | Previous loop's height, for d(alt)/dt |
| `velocityZ` | `float` | Vertical velocity (m/s), differentiated from the barometric height |
| `currentTime`, `lastTime` | `unsigned long` | Timestamps for delta-time calculation |
| `deltaTime` | `float` | Elapsed time since last loop iteration (ms) |

---

### `src/sensors.cpp`
**Low-level sensor readout functions.**

| Function | Description |
|---|---|
| `readDPS310()` | Polls the DPS310 for temperature and pressure when both are available. Stores them in `temperature_C` and `pressure_hPa`, maintains the 3-deep rolling buffers and their averages, latches `groundPressure_hPa` from the first reading, then calls `updateAltitude()`. |
| `readBNO08x()` | **Commented out** in its entirety along with the BNO085. |

---

### `src/brains.cpp`
**Flight logic — height estimation, state machine, and data smoothing.**

| Function | Description |
|---|---|
| `calculateAverages()` | Averages the first `size` entries of a 3-element rolling buffer. Used for pressure, temperature, and altitude. |
| `updateAltitude()` | Converts the smoothed pressure into height above the pad using the international barometric formula `h = 44330 * (1 - (P/P0)^(1/5.255))`, where `P0 = groundPressure_hPa`. Maintains a 3-deep rolling average in `altitude_m_avg` and tracks `maxAltitude_m`. |
| `calculateVelocityZ()` | Differentiates the smoothed height: `velocityZ = (altitude_m_avg - lastAltitude_m) / dt`, with `dt = deltaTime / 1000`. This replaces the old accelerometer integration. |
| `figureOutState()` | Forward-only (never steps backwards) state machine driven by height and climb rate: |
| | • **STATE_BOOST** — `altitude_m_avg > 3 m` AND `velocityZ > 0.5 m/s` |
| | • **STATE_COAST** — `velocityZ < 5 m/s` (climb rate falling off, standing in for burnout) |
| | • **STATE_APOGEE** — `velocityZ <= 0.5 m/s` |
| | • **STATE_DESCENT** — `velocityZ < -0.5 m/s` |
| | • **STATE_LANDED** — `|velocityZ| < 0.5 m/s` AND `altitude_m_avg < 2 m` |

---

### `src/diag.cpp`
**Raw-SPI diagnostic console**, compiled only with `-D DIAGNOSTIC_MODE` (the `nano_every_diag` env). Talks to the DPS310 with raw `SPI.transfer()` calls so you can inspect the bus directly. The BNO085 probes (`int`, `bnoreset`, and auto-diagnostic steps 5–7) are **commented out**.

---

## Header Files (`include/`)

### `include/globals.hpp`
**Global declarations, constants, pin mappings, and state enum.**
- Defines SPI pin assignments: `DPS310_CS = 3` (D3), `SD_CS = 2` (D2). Nano Every default SPI: MOSI = 11, MISO = 12, SCK = 13.
- The BNO085 pins (`BNO08X_CS`, `BNO08X_INT`, `BNO08X_RESET`) and the `Adafruit_BNO08x.h` include are **commented out**.
- Defines flight state constants: `STATE_STANDBY(0)`, `STATE_BOOST(1)`, `STATE_COAST(2)`, `STATE_APOGEE(3)`, `STATE_DESCENT(4)`, `STATE_LANDED(5)`.
- Declares all global variables with `extern` (mirrors `globals.cpp`).

### `include/brains.hpp`
**Declares `calculateAverages()`, `updateAltitude()`, `figureOutState()`, and `calculateVelocityZ()`.**

### `include/sensors.hpp`
**Declares `readDPS310()` and `writeSD()`.** The `readBNO08x()` declaration is commented out.

---

## Known Issues / TODOs

1. **`writeSD()` is a stub.** No data logging to SD card yet.
2. **No initial standby/armed detection.** The first state check immediately jumps to boost if conditions are met.
3. **No abort or safe state handling.** `STATE_STANDBY` is the default but never explicitly set by logic.
4. **Height resolution is limited.** The DPS310's noise floor limits sub-metre accuracy, and `velocityZ` is obtained by differentiating a smoothed height, so it lags and is noisier than an accelerometer integration would be. Flight thresholds may need tuning.
5. **`groundPressure_hPa` is latched from the first reading after boot** — the rocket must be sitting on the pad (not being carried around) when the board powers on, and weather drift during a long wait on the pad is not compensated.
6. **Docs still reference the BNO085** (README wiring tables, Google Doc).
