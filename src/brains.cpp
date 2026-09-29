#include "brains.hpp"
#include "glob.h"
extern volatile bool dps_data_ready;


void calculateAverages(){       
    // TODO store last 3 data readings for all important variables (eg accelZ)
    // and create an average for those last 3, called accelZ_avg.
    // We will use that data to figure out what state we're in
    // NOTE: Dont create averages for ALL the data readings (for example,
    // we don't need a smoothed average for temperature readings)
    
    // create lists for last 3 readings of each important variable
    static float accelX_list[3] = {0.0f, 0.0f, 0.0f};
    static float accelY_list[3] = {0.0f, 0.0f, 0.0f};
    static float accelZ_list[3] = {0.0f, 0.0f, 0.0f};
    static float gyroX_list[3] = {0.0f, 0.0f, 0.0f};
    static float gyroY_list[3] = {0.0f, 0.0f, 0.0f};
    static float gyroZ_list[3] = {0.0f, 0.0f, 0.0f};
    static float magX_list[3] = {0.0f, 0.0f, 0.0f};
    static float magY_list[3] = {0.0f, 0.0f, 0.0f};
    static float magZ_list[3] = {0.0f, 0.0f, 0.0f};
    static float quatReal_list[3] = {0.0f, 0.0f, 0.0f};
    static float quatI_list[3] = {0.0f, 0.0f, 0.0f};
    static float quatJ_list[3] = {0.0f, 0.0f, 0.0f};
    static float quatK_list[3] = {0.0f, 0.0f, 0.0f};
    static float temperature_C_list[3] = {0.0f, 0.0f, 0.0f};
    static float pressure_hPa_list[3] = {0.0f, 0.0f, 0.0f};
    
    // create index variables to keep track of where to insert new readings
    static int accel_index = 0;
    static int gyro_index = 0;
    static int mag_index = 0;
    static int quat_index = 0;
    static int temperature_index = 0;
    static int pressure_index = 0;

    // create a divisor variable to divide the sum of the last 3 readings by 3 to get the average
    static int accel_divisor = 0;
    static int gyro_divisor = 0;
    static int mag_divisor = 0;
    static int quat_divisor = 0;
    static int temperature_divisor = 0;
    static int pressure_divisor = 0;

    // shift readings over and add new reading to the end of the list
    if(sensorValue.sensorId == SH2_ACCELEROMETER){
   
    //assign new readings to the lists
    accelX_list[accel_index] = accelX;
    accelY_list[accel_index] = accelY;
    accelZ_list[accel_index] = accelZ;

    //Calculate averages for the acceleration readings
    //update divisor for new reading, stop at 3
    if(accel_divisor < 3) {
        accel_divisor++;
    }
    accelX_avg = (accelX_list[0] + accelX_list[1] + accelX_list[2]) / accel_divisor;
    accelY_avg = (accelY_list[0] + accelY_list[1] + accelY_list[2]) / accel_divisor;
    accelZ_avg = (accelZ_list[0] + accelZ_list[1] + accelZ_list[2]) / accel_divisor;

    //update index for new reading, wrap around if it reaches 3
    accel_index = (accel_index + 1) % 3;

}   else if(sensorValue.sensorId == SH2_GYROSCOPE_CALIBRATED){
    
    //assign new readings to the lists
    gyroX_list[gyro_index] = gyroX;
    gyroY_list[gyro_index] = gyroY;
    gyroZ_list[gyro_index] = gyroZ;

    //Calculate the averages for the gyro readings
    //update divisor for new reading, stop at 3
    if(gyro_divisor < 3) {
        gyro_divisor++;
    }
    gyroX_avg = (gyroX_list[0] + gyroX_list[1] + gyroX_list[2]) / gyro_divisor;
    gyroY_avg = (gyroY_list[0] + gyroY_list[1] + gyroY_list[2]) / gyro_divisor;
    gyroZ_avg = (gyroZ_list[0] + gyroZ_list[1] + gyroZ_list[2]) / gyro_divisor;

    //update index for new reading, wrap around if it reaches 3
    gyro_index = (gyro_index + 1) % 3;


}   else if(sensorValue.sensorId == SH2_MAGNETIC_FIELD_CALIBRATED){
    
    //assign new readings to the lists
    magX_list[mag_index] = magX;
    magY_list[mag_index] = magY;
    magZ_list[mag_index] = magZ;

    //Calculate the averages for the mag readings
    //update divisor for new reading, stop at 3
    if(mag_divisor < 3) {
        mag_divisor++;
    }
    magX_avg = (magX_list[0] + magX_list[1] + magX_list[2]) / mag_divisor;
    magY_avg = (magY_list[0] + magY_list[1] + magY_list[2]) / mag_divisor;
    magZ_avg = (magZ_list[0] + magZ_list[1] + magZ_list[2]) / mag_divisor;

    //update index for new reading, wrap around if it reaches 3
    mag_index = (mag_index + 1) % 3;

}   else if(sensorValue.sensorId == SH2_ROTATION_VECTOR){
   
    //assign new readings to the lists
    quatReal_list[quat_index] = quatReal;
    quatI_list[quat_index] = quatI;
    quatJ_list[quat_index] = quatJ;
    quatK_list[quat_index] = quatK;


    //Calculate the averages for the quat readings
    //update divisor for new reading, stop at 3
    if(quat_divisor < 3) {
        quat_divisor++;
    }
    quatReal_avg = (quatReal_list[0] + quatReal_list[1] + quatReal_list[2]) / quat_divisor;
    quatI_avg = (quatI_list[0] + quatI_list[1] + quatI_list[2]) / quat_divisor;
    quatJ_avg = (quatJ_list[0] + quatJ_list[1] + quatJ_list[2]) / quat_divisor;
    quatK_avg = (quatK_list[0] + quatK_list[1] + quatK_list[2]) / quat_divisor;

    //update index for new reading, wrap around if it reaches 3
    quat_index = (quat_index + 1) % 3;

}   else if(dps_data_ready == true) { 
    // Check if temperature and pressure readings are valid
    temperature_C_list[temperature_index] = temperature_C;
    if(temperature_divisor < 3) temperature_divisor++;
    temperature_C_avg = (temperature_C_list[0] + temperature_C_list[1] + temperature_C_list[2]) / temperature_divisor;
    temperature_index = (temperature_index + 1) % 3;

    pressure_hPa_list[pressure_index] = pressure_hPa;
    if(pressure_divisor < 3) pressure_divisor++;
    pressure_hPa_avg = (pressure_hPa_list[0] + pressure_hPa_list[1] + pressure_hPa_list[2]) / pressure_divisor;
    pressure_index = (pressure_index + 1) % 3;

    dps_data_ready = false;  // <-- consume the flag so stale data isn't reprocessed
}
}

void figureOutState(){

    /*
    Standby: Power is on, systems are idle, and the rocket sits on the pad waiting for final arming.
    Armed: Launch checks are complete, pyro channels are live, and the flight computer actively listens for launch acceleration.
    Boost (Launch): The motor ignites and the rocket accelerates upward off the pad, detected by high G-force and positive velocity.
    Coast: Motor burnout occurs, and the rocket continues climbing upward on momentum until it reaches apogee.
    Apogee: The highest point of the flight where vertical velocity hits zero, which triggers the primary deployment event (drogue parachute or main depending on dual-deploy).
    Recovery / Descent: The parachutes deploy, and the rocket floats safely back down to the ground.
    Touchdown: The rocket lands, detected by zero movement and zero vertical velocity, signaling the end of the flight profile.
    
    Abort: An anomaly is detected on the pad or early in flight, shutting down or safely triggering recovery before disaster strikes.
    Safe: A post-flight or error state where charges are disabled so recovery crews can handle the rocket safely.
    */
    

    
    /*
    if (Power is on, systems are idle, rocket is on pad) {
    state = STATE_STANDBY;
    }
    */
    
    if (accelZ_avg >= 20 && velocityZ>1) {
        state = STATE_BOOST;
    }

    if (accelZ_avg < 1 && velocityZ>0) {
        state = STATE_COAST;
    }

    if (velocityZ <= 1 && velocityZ >= -1) {
        state = STATE_APOGEE;
    }

    if (velocityZ < -1) {
        state = STATE_DESCENT;
    }

    if (accelZ_avg < 1 && velocityZ < 1) {
        state = STATE_LANDED;
    }   

}

void calculateVelocityZ(){
    velocityZ += accelZ_avg * deltaTime;
}
