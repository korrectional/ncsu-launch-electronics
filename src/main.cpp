#include "globals.hpp"
#include "brains.hpp"
#include "sensors.hpp"



void setup() {
    Serial.begin(115200);
    delay(1000);

    Wire.begin();

    // Init BNO08x
    if (!bno08x.begin_I2C()) {
        Serial.println("Failed to find BNO085!");
        while (1) delay(10);
    }
    Serial.println("BNO085 found!");

    bno08x.enableReport(SH2_ACCELEROMETER);
    bno08x.enableReport(SH2_GYROSCOPE_CALIBRATED);
    bno08x.enableReport(SH2_MAGNETIC_FIELD_CALIBRATED);
    bno08x.enableReport(SH2_ROTATION_VECTOR);

    // Init DPS310
    if (!dps.begin_I2C()) {
        Serial.println("Failed to find DPS310!");
        while (1) delay(10);
    }
    Serial.println("DPS310 found!");

    dps.configurePressure(DPS310_64HZ, DPS310_64SAMPLES);
    dps.configureTemperature(DPS310_64HZ, DPS310_64SAMPLES);

    Serial.println("Sensors ready.");

    lastTime = millis();
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
    // accelX/Y/Z, gyroX/Y/Z, magX/Y/Z, quatReal/I/J/K, pressure_hPa, temperature_C
    // as well as their averaged counterparts

    for (int i = 0; i < 3; i++) {
        readBNO08x();
        readDPS310();
    }
    readBNO08x();
    readDPS310();

    





    figureOutState();
    writeSD();



    currentTime = millis();
    deltaTime = (currentTime - lastTime); 
    lastTime = currentTime;


    delay(50);
}
