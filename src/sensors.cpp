#include "sensors.hpp"
#include "globals.hpp"
#include "brains.hpp"

void readBNO08x() {
    if (bno08x.wasReset()) {
        Serial.println("BNO085 reset!");
    }

    if (bno08x.getSensorEvent(&sensorValue)) {
        switch (sensorValue.sensorId) {
            case SH2_ACCELEROMETER:
                accelX = sensorValue.un.accelerometer.x;
                accelY = sensorValue.un.accelerometer.y;
                accelZ = sensorValue.un.accelerometer.z;
                //create variable and set up circular logic buffer to store last 3 readings of accelX/Y/Z 
                //and then calculate average of those 3 readings
                static int accelIndex = 0;
                accelx_list[accelIndex] = accelX;
                accely_list[accelIndex] = accelY;
                accelz_list[accelIndex] = accelZ;
          
                accelIndex = (accelIndex + 1) % 3;
                //set up condition to check how many readings have been taken and find average
                static int accelReadings = 1;
                if(accelReadings >= 3){
                    accelX_avg = calculateAverages(accelx_list, 3);
                    accelY_avg = calculateAverages(accely_list, 3);
                    accelZ_avg = calculateAverages(accelz_list, 3);
                } else if(accelReadings == 2){
                    accelX_avg = calculateAverages(accelx_list, 2);
                    accelY_avg = calculateAverages(accely_list, 2);
                    accelZ_avg = calculateAverages(accelz_list, 2);
                    accelReadings++;
                } else if(accelReadings == 1){
                    accelX_avg = accelX;
                    accelY_avg = accelY;
                    accelZ_avg = accelZ;
                    accelReadings++;
                }
                break;
            case SH2_GYROSCOPE_CALIBRATED:
                gyroX = sensorValue.un.gyroscope.x;
                gyroY = sensorValue.un.gyroscope.y;
                gyroZ = sensorValue.un.gyroscope.z;
                //create variable and set up circular logic buffer to store last 3 readings of accelX/Y/Z 
                //and then calculate average of those 3 readings
                static int gyroIndex = 0;
                gyrox_list[gyroIndex] = gyroX;
                gyroy_list[gyroIndex] = gyroY;
                gyroz_list[gyroIndex] = gyroZ;
          
                gyroIndex = (gyroIndex + 1) % 3;
                //set up condition to check how many readings have been taken and find average
                static int gyroReadings = 1;
                if(gyroReadings >= 3){
                    gyroX_avg = calculateAverages(gyrox_list, 3);
                    gyroY_avg = calculateAverages(gyroy_list, 3);
                    gyroZ_avg = calculateAverages(gyroz_list, 3);
                } else if(gyroReadings == 2){
                    gyroX_avg = calculateAverages(gyrox_list, 2);
                    gyroY_avg = calculateAverages(gyroy_list, 2);
                    gyroZ_avg = calculateAverages(gyroz_list, 2);
                    gyroReadings++;
                } else if(gyroReadings == 1){
                    gyroX_avg = gyroX;
                    gyroY_avg = gyroY;
                    gyroZ_avg = gyroZ;
                    gyroReadings++;
                }
                break;
            case SH2_MAGNETIC_FIELD_CALIBRATED:
                magX = sensorValue.un.magneticField.x;
                magY = sensorValue.un.magneticField.y;
                magZ = sensorValue.un.magneticField.z;
                //create variable and set up circular logic buffer to store last 3 readings of accelX/Y/Z 
                //and then calculate average of those 3 readings
                static int MagIndex = 0;
                magx_list[MagIndex] = magX;
                magy_list[MagIndex] = magY;
                magz_list[MagIndex] = magZ;
          
                MagIndex = (MagIndex + 1) % 3;
                //set up condition to check how many readings have been taken and find average
                static int magReadings = 1;
                if(magReadings >= 3){
                    magX_avg = calculateAverages(magx_list, 3);
                    magY_avg = calculateAverages(magy_list, 3);
                    magZ_avg = calculateAverages(magz_list, 3);
                } else if(magReadings == 2){
                    magX_avg = calculateAverages(magx_list, 2);
                    magY_avg = calculateAverages(magy_list, 2);
                    magZ_avg = calculateAverages(magz_list, 2);
                    magReadings++;
                } else if(magReadings == 1){
                    magX_avg = magX;
                    magY_avg = magY;
                    magZ_avg = magZ;
                    magReadings++;
                }
                break;
            case SH2_ROTATION_VECTOR:
                quatReal = sensorValue.un.rotationVector.real;
                quatI = sensorValue.un.rotationVector.i;
                quatJ = sensorValue.un.rotationVector.j;
                quatK = sensorValue.un.rotationVector.k;
                //create variable and set up circular logic buffer to store last 3 readings of accelX/Y/Z 
                //and then calculate average of those 3 readings
                static int quatIndex = 0;
                quatReal_list[quatIndex] = quatReal;
                quatI_list[quatIndex] = quatI;
                quatJ_list[quatIndex] = quatJ;
                quatK_list[quatIndex] = quatK;

                quatIndex = (quatIndex + 1) % 3;
                //set up condition to check how many readings have been taken and find average
                static int quatReadings = 1;
                if(quatReadings >= 3){
                    quatReal_avg = calculateAverages(quatReal_list, 3);
                    quatI_avg = calculateAverages(quatI_list, 3);
                    quatJ_avg = calculateAverages(quatJ_list, 3);
                    quatK_avg = calculateAverages(quatK_list, 3);
                } else if(quatReadings == 2){
                    quatReal_avg = calculateAverages(quatReal_list, 2);
                    quatI_avg = calculateAverages(quatI_list, 2);
                    quatJ_avg = calculateAverages(quatJ_list, 2);
                    quatK_avg = calculateAverages(quatK_list, 2);
                    quatReadings++;
                } else if(quatReadings == 1){
                    quatReal_avg = quatReal;
                    quatI_avg = quatI;
                    quatJ_avg = quatJ;
                    quatK_avg = quatK;
                    quatReadings++;
                }
                break;
        }
    }
}

void readDPS310() {
    sensors_event_t temp_event, pressure_event;
    if (dps.temperatureAvailable() && dps.pressureAvailable()) {
        dps.getEvents(&temp_event, &pressure_event);
        temperature_C = temp_event.temperature;
        pressure_hPa = pressure_event.pressure;
        //create variable and set up circular logic buffer to store last 3 readings of accelX/Y/Z 
        //and then calculate average of those 3 readings
        static int temp_pressureIndex = 0;
        temperature_list[temp_pressureIndex] = temperature_C;
        pressure_list[temp_pressureIndex] = pressure_hPa;
        temp_pressureIndex = (temp_pressureIndex + 1) % 3;
        //set up condition to check how many readings have been taken and find average
        static int temp_pressureReadings = 1;
        if(temp_pressureReadings >= 3){
            temperature_C_avg = calculateAverages(temperature_list, 3);
            pressure_hPa_avg = calculateAverages(pressure_list, 3);
        } else if(temp_pressureReadings == 2){
            temperature_C_avg = calculateAverages(temperature_list, 2);
            pressure_hPa_avg = calculateAverages(pressure_list, 2);
            temp_pressureReadings++;
        } else if(temp_pressureReadings == 1){
            temperature_C_avg = temperature_C;
            pressure_hPa_avg = pressure_hPa;
            temp_pressureReadings++;
        }
    }
}
