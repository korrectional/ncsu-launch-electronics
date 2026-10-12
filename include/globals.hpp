#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_BNO08x.h>
#include <Adafruit_DPS310.h>

// SPI chip-select / control pins (Nano Every default SPI: MOSI=11, MISO=12, SCK=13)
// The megaAVR Arduino core does NOT define D0..D13 macros, so use the numeric
// Arduino pin index (the number silkscreened next to "Dx").
#define BNO08X_CS    5  // D5  - BNO085 chip select
#define BNO08X_INT   6  // D6  - BNO085 active-low data-ready interrupt (required for SPI)
#define BNO08X_RESET 7  // D7  - BNO085 active-low reset (required for SPI)
#define DPS310_CS    3  // D3  - DPS310 chip select
#define SD_CS        2  // D2  - SD-card reader chip select (writeSD logging)

#define STATE_STANDBY 0
#define STATE_BOOST 1
#define STATE_COAST 2
#define STATE_APOGEE 3
#define STATE_DESCENT 4
#define STATE_LANDED 5
extern int state;

extern Adafruit_BNO08x bno08x;
extern sh2_SensorValue_t sensorValue;
extern Adafruit_DPS310 dps;

// Stored sensor variables
extern float accelX, accelY, accelZ;
extern float gyroX, gyroY, gyroZ;
extern float magX, magY, magZ;
extern float quatReal, quatI, quatJ, quatK;
extern float pressure_hPa;
extern float temperature_C;

// Averages over last couple cycles, we use them to smoothe out data
extern float accelX_avg, accelY_avg, accelZ_avg;
extern float gyroX_avg, gyroY_avg, gyroZ_avg;
extern float magX_avg, magY_avg, magZ_avg;
extern float quatReal_avg, quatI_avg, quatJ_avg, quatK_avg;
extern float pressure_hPa_avg;
extern float temperature_C_avg;


extern float velocityZ;

extern unsigned long currentTime;
extern unsigned long lastTime;
extern float deltaTime;

extern float accelx_list[3];
extern float accely_list[3];
extern float accelz_list[3];
extern float gyrox_list[3];
extern float gyroy_list[3];
extern float gyroz_list[3];
extern float magx_list[3];
extern float magy_list[3];
extern float magz_list[3];
extern float quatReal_list[3];
extern float quatI_list[3];
extern float quatJ_list[3];
extern float quatK_list[3];
extern float pressure_list[3];
extern float temperature_list[3];
