#include "globals.hpp"
#include <Adafruit_BNO08x.h>
#include <Adafruit_DPS310.h>

int state = STATE_STANDBY;

Adafruit_BNO08x bno08x(BNO08X_RESET);
sh2_SensorValue_t sensorValue;
Adafruit_DPS310 dps;

float accelX, accelY, accelZ;

float gyroX, gyroY, gyroZ;
float magX, magY, magZ;
float quatReal, quatI, quatJ, quatK;
float pressure_hPa;
float temperature_C;

float accelX_avg, accelY_avg, accelZ_avg; // m/s2
float gyroX_avg, gyroY_avg, gyroZ_avg;
float magX_avg, magY_avg, magZ_avg;
float quatReal_avg, quatI_avg, quatJ_avg, quatK_avg;
float pressure_hPa_avg;
float temperature_C_avg;

float velocityZ;

unsigned long currentTime = 0;
unsigned long lastTime = 0;
float deltaTime = 0.0f;

float accelx_list[3] = {0.0f, 0.0f, 0.0f};
float accely_list[3] = {0.0f, 0.0f, 0.0f};
float accelz_list[3] = {0.0f, 0.0f, 0.0f};
float gyrox_list[3] = {0.0f, 0.0f, 0.0f};
float gyroy_list[3] = {0.0f, 0.0f, 0.0f};
float gyroz_list[3] = {0.0f, 0.0f, 0.0f};
float magx_list[3] = {0.0f, 0.0f, 0.0f};
float magy_list[3] = {0.0f, 0.0f, 0.0f};
float magz_list[3] = {0.0f, 0.0f, 0.0f};
float quatReal_list[3] = {0.0f, 0.0f, 0.0f};
float quatI_list[3] = {0.0f, 0.0f, 0.0f};
float quatJ_list[3] = {0.0f, 0.0f, 0.0f};
float quatK_list[3] = {0.0f, 0.0f, 0.0f};
float pressure_list[3] = {0.0f, 0.0f, 0.0f};
float temperature_list[3] = {0.0f, 0.0f, 0.0f};
