Our cool rocket project!!!

Our general doc:
https://docs.google.com/document/d/1P8TpsTtHFG3OcJ_iXiIPnqLnQIX96XdeIowkqV0P21Y/edit?usp=sharing

---

## Wiring (SPI)

The BNO085 IMU and DPS310 barometer share one SPI bus and each get their own
chip-select (CS) line.

### Power

Both sensor breakouts are powered from the Nano 33 BLE's **3V3** pin. This is
required, not just convenient: each breakout sets the logic level of its header
pins to whatever voltage is on its `VIN` pin, so `VIN` must be 3.3V to match the
Nano's 3.3V (non-5V-tolerant) I/O.

| Breakout pin | Wire to | Notes |
|---|---|---|
| `VIN` | Nano `3V3` | Sets both the supply and the header logic level (3.3V) |
| `GND` | Nano `GND` | Common ground |
| `3Vo` | *unconnected* | Regulator output (`3Vo` is an output, not an input); only used if powering another 3.3V device |

> **Warning:** do **not** wire the breakout `VIN` to 5V or to the Nano's `VIN`
> pin. The breakout's data pins (notably the BNO085's SDA/MISO) would then swing
> to ~5V into the Nano's 3.3V-only, non-5V-tolerant inputs.

### Shared SPI bus

The Nano 33 BLE's default SPI pins are used for the shared bus:

| Shared SPI signal | Nano 33 BLE pin |
|---|---|
| MOSI (COPI / DI / SDI) | D11 |
| MISO (CIPO / SDA / SDO) | D12 |
| SCK | D13 |

### Chip select and control pins

| Device | Signal | Nano 33 BLE pin | Notes |
|---|---|---|---|
| BNO085 | CS | D5 | `BNO08X_CS` — drop low to talk to the BNO085 |
| BNO085 | INT | D6 | `BNO08X_INT` — active-low data-ready; **required** for SPI |
| BNO085 | RST | D7 | `BNO08X_RESET` — active-low reset; wire for stable SPI |
| DPS310 | CS | D3 | `DPS310_CS` — drop low to talk to the DPS310 |
| SD card | CS | D2 | `SD_CS` — chip select for `writeSD()` logging |

### DPS310 wiring (explicit)

| DPS310 breakout pin | Nano pin | Notes |
|---|---|---|
| `VIN` | `5V` | sets the header logic level (5V for the Nano Every) |
| `GND` | `GND` | common ground |
| `SCK` | `D13` | SPI clock |
| `SDI` | `D11` | MOSI (Nano -> sensor) |
| `SDO` | `D12` | MISO (sensor -> Nano) |
| `CS` | `D3` | chip select (`DPS310_CS`) |
| `3Vo` | *unconnected* | regulator output, leave open |

> **BNO085 mode jumpers:** the breakout defaults to I2C. To use SPI you must
> bridge **both** the `P0` and `P1` mode-select solder jumpers on the back of
> the board to **3V** (PS1 = High, PS0 = High = SPI).




d2- SD reader CS
d3- DSP CS
d4- none
d5- BNO CS
d6- BNO INT
d7- BNO RST
d8- SD reader CD