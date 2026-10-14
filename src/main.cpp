#include "globals.hpp"
#include "brains.hpp"
#include "sensors.hpp"

// Sensor bring-up state. If the DPS310 is not detected we do NOT hang; we keep
// retrying every SENSOR_RETRY_MS until it answers. The flight/log loop starts as
// soon as the DPS310 is up.
//
// NOTE: the BNO085 IMU has been dropped from this design. The DPS310 barometer
// is the only sensor, and its pressure reading is what we turn into height.
static const unsigned long SENSOR_RETRY_MS = 2000;
static bool sensorsReady = false;
static bool dpsReady = false;
static unsigned long lastSensorAttempt = 0;

// -------------------------------------------------------------------------
// Bring-up diagnostics
// -------------------------------------------------------------------------
// Raw single-register read over SPI. The DPS310 control byte sets bit 7 for a
// read, and the chip latches into SPI mode once CS goes low.
static uint8_t dpsRawRead(uint8_t reg, uint8_t mode, uint32_t hz) {
    SPI.beginTransaction(SPISettings(hz, MSBFIRST, mode));
    digitalWrite(DPS310_CS, LOW);
    delayMicroseconds(5);
    SPI.transfer(reg | 0x80);
    uint8_t value = SPI.transfer(0x00);
    digitalWrite(DPS310_CS, HIGH);
    SPI.endTransaction();
    return value;
}

// Probe the Nano's own SPI master/MISO path. With a jumper between D11 (MOSI)
// and D12 (MISO) this must print 0xA5; if it prints 0xFF the Nano side (or the
// MISO wire) is the problem, not the sensors.
static void debugSPI() {
    // BNO085 chip select no longer driven; only the devices we still use.
    // pinMode(BNO08X_CS, OUTPUT); digitalWrite(BNO08X_CS, HIGH);
    pinMode(DPS310_CS, OUTPUT); digitalWrite(DPS310_CS, HIGH);
    pinMode(SD_CS, OUTPUT);     digitalWrite(SD_CS, HIGH);

    Serial.print("MISO (D12) idle level: ");
    Serial.println(digitalRead(12));
}

// -------------------------------------------------------------------------
// BNO085 bring-up -- DISABLED
// -------------------------------------------------------------------------
// The IMU and its driver are no longer used. Kept commented out for reference.
//
// Pulse the BNO085 reset and watch its INT line (active low). In SPI mode the
// chip asserts INT shortly after reset to announce itself. If INT never goes
// low, the chip is either not running in SPI mode (PS0/PS1 not both high at
// reset), not powered, or INT/RST is not wired.
//
// NOTE: with a jumper between D11 (MOSI) and D12 (MISO) the loopback below must
// print 0xA5; anything else means the Nano's own SPI/MISO path is the problem.
/*
static void debugBNO() {
    // Loopback probe of the Nano's SPI master (requires the D11-D12 jumper).
    pinMode(11, OUTPUT);
    pinMode(12, INPUT);
    digitalWrite(11, HIGH);
    delayMicroseconds(10);
    int loopHigh = digitalRead(12);
    digitalWrite(11, LOW);
    delayMicroseconds(10);
    int loopLow = digitalRead(12);
    Serial.print("SPI loopback (needs D11-D12 jumper): high=");
    Serial.print(loopHigh);
    Serial.print(" low=");
    Serial.println(loopLow);

    pinMode(BNO08X_INT, INPUT_PULLUP);
    Serial.print("BNO INT (D6) idle level: ");
    Serial.println(digitalRead(BNO08X_INT));

    // Hardware reset: PS0/PS1 are latched here, so they must already be high.
    pinMode(BNO08X_RESET, OUTPUT);
    digitalWrite(BNO08X_RESET, HIGH);
    delay(10);
    digitalWrite(BNO08X_RESET, LOW);
    delay(10);
    digitalWrite(BNO08X_RESET, HIGH);

    unsigned long t0 = millis();
    while (digitalRead(BNO08X_INT) == HIGH && (millis() - t0) < 1000) {
        // wait up to 1 s for the chip to drive INT low
    }
    if (digitalRead(BNO08X_INT) == LOW) {
        Serial.print("BNO asserted INT after reset (chip alive in SPI mode) after ");
        Serial.print(millis() - t0);
        Serial.println(" ms.");
    } else {
        Serial.println("BNO never asserted INT -> not in SPI mode / not powered / INT not wired.");
    }
}

static bool tryInitBNO() {
    if (bnoReady) return true;

    bool ok = bno08x.begin_SPI(BNO08X_CS, BNO08X_INT);
    if (ok) {
        bno08x.enableReport(SH2_ACCELEROMETER);
        bno08x.enableReport(SH2_GYROSCOPE_CALIBRATED);
        bno08x.enableReport(SH2_MAGNETIC_FIELD_CALIBRATED);
        bno08x.enableReport(SH2_ROTATION_VECTOR);
    }
    Serial.print("BNO085 SPI: ");
    Serial.println(ok ? "OK" : "FAILED");
    bnoReady = ok;
    return ok;
}
*/

static bool tryInitDPS() {
    if (dpsReady) return true;

    bool ok = dps.begin_SPI(DPS310_CS);
    if (ok) {
        dps.configurePressure(DPS310_64HZ, DPS310_64SAMPLES);
        dps.configureTemperature(DPS310_64HZ, DPS310_64SAMPLES);
    } else {
        // Dump the raw ID register to help diagnose wiring / mode issues.
        uint8_t id0 = dpsRawRead(0x0D, SPI_MODE0, 1000000);
        uint8_t id3 = dpsRawRead(0x0D, SPI_MODE3, 1000000);
        Serial.print("  raw reg 0x0D (expect 0x10) mode0=0x");
        Serial.print(id0, HEX);
        Serial.print(" mode3=0x");
        Serial.println(id3, HEX);
    }
    Serial.print("DPS310 SPI: ");
    Serial.println(ok ? "OK" : "FAILED");
    dpsReady = ok;
    return ok;
}

bool initSensors() {
    // tryInitBNO();  // BNO085 removed
    return tryInitDPS();   // the DPS is all we need to start the loop
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("Booting...");

    // Park every chip-select high before we touch the bus. An unselected
    // device must not be able to drive MISO while we talk to another device.
    // pinMode(BNO08X_CS, OUTPUT); digitalWrite(BNO08X_CS, HIGH);  // BNO085 removed
    pinMode(DPS310_CS, OUTPUT); digitalWrite(DPS310_CS, HIGH);
    pinMode(SD_CS, OUTPUT);     digitalWrite(SD_CS, HIGH);

    SPI.begin();

    debugSPI();
    // debugBNO();  // BNO085 removed

    lastSensorAttempt = millis();
    sensorsReady = initSensors();
    if (sensorsReady) {
        Serial.println("Sensors ready.");
        lastTime = millis();
    } else {
        Serial.println("Sensor init failed - will retry every 2 s.");
    }
}



void writeSD() {
    // TODO: write data into SD card
    // I actually don't want this to be happening super fast, maybe a rate of
    //  20Hz to 50Hz would be good
    // This function will require some knowledge about buffering and flushing
    // in SD cards, so start researching!
}




void loop() {
    // Available Variables:
    // pressure_hPa, temperature_C as well as their averaged counterparts,
    // plus altitude_m / altitude_m_avg / velocityZ derived from the pressure.
    // (The BNO085's accel/gyro/mag/quat readings are no longer read.)

    // Bring-up phase: keep retrying until the DPS310 is up.
    if (!sensorsReady) {
        if (millis() - lastSensorAttempt >= SENSOR_RETRY_MS) {
            lastSensorAttempt = millis();
            Serial.println("Retrying sensor init...");
            sensorsReady = initSensors();
            if (sensorsReady) {
                Serial.println("Sensors ready.");
                lastTime = millis();
            }
        }
        return;
    }

    // Time since the previous loop iteration, in milliseconds.
    currentTime = millis();
    deltaTime = (float)(currentTime - lastTime);
    lastTime = currentTime;

    // Take a few samples so the 3-deep rolling averages have fresh data.
    for (int i = 0; i < 4; i++) {
        // if (bnoReady) readBNO08x();  // BNO085 removed
        readDPS310();
    }

    // Differentiate the height into vertical velocity, then classify the flight.
    calculateVelocityZ();
    figureOutState();
    writeSD();

    // Print live sensor readings ~2x per second so each can be validated.
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint >= 500) {
        lastPrint = millis();

        Serial.print("DPS:  T = ");
        Serial.print(temperature_C, 2);
        Serial.print(" C   P = ");
        Serial.print(pressure_hPa, 2);
        Serial.print(" hPa   | avg T = ");
        Serial.print(temperature_C_avg, 2);
        Serial.print(" C, avg P = ");
        Serial.print(pressure_hPa_avg, 2);
        Serial.print(" hPa");
        Serial.println();

        // Barometric height -- this is the number the flight logic runs on.
        Serial.print("ALT:  altitude_m = ");
        Serial.print(altitude_m, 2);
        Serial.print(" m   altitude_m_avg = ");
        Serial.print(altitude_m_avg, 2);
        Serial.print(" m   maxAltitude_m = ");
        Serial.print(maxAltitude_m, 2);
        Serial.print(" m");
        Serial.println();

        // Derived climb rate, pad reference pressure, and current flight state.
        Serial.print("ALT:  vZ = ");
        Serial.print(velocityZ, 2);
        Serial.print(" m/s   groundPressure_hPa = ");
        Serial.print(groundPressure_hPa, 2);
        Serial.print(" hPa   state = ");
        Serial.println(state);
    }

    delay(50);
}
