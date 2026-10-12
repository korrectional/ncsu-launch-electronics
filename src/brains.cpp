#include "brains.hpp"
#include "globals.hpp"

#include <math.h>

// Flight thresholds -- tune these to the vehicle.
// The accelerometer reports proper acceleration, so with the rocket's +Z axis
// pointing up it reads about +9.81 m/s^2 while the rocket sits on the pad.
static constexpr float GRAVITY       = 9.81f;  // m/s^2
static constexpr float LAUNCH_ACCEL  = 20.0f;  // m/s^2, boost detection
static constexpr float BURNOUT_ACCEL = 1.0f;   // m/s^2, "thrust is gone"
static constexpr float VELOCITY_EPS  = 1.0f;   // m/s, near-zero velocity


float calculateAverages(float list[3], int size){       
    // TODO store last 3 data readings for all important variables (eg accelZ)
    // and create an average for those last 3, called accelZ_avg.
    // We will use that data to figure out what state we're in
    // NOTE: Dont create averages for ALL the data readings (for example,
    // we don't need a smoothed average for temperature readings)
    
    // create lists for last 3 readings of each important variable
    float returnVal = 0;
    for (int i = 0; i < size; i++) {
        returnVal += list[i];
    }
    returnVal /= size;
    return returnVal;
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
    
    // Forward-only state machine: once a flight phase is entered it never
    // steps backwards, so a noisy reading cannot rewind the flight.
    switch (state) {
        case STATE_STANDBY:
            // On the pad until real thrust shows up.
            if (accelZ_avg >= LAUNCH_ACCEL && velocityZ > VELOCITY_EPS) {
                state = STATE_BOOST;
            }
            break;

        case STATE_BOOST:
            // Burnout: thrust is gone, but the rocket is still climbing.
            if (accelZ_avg < BURNOUT_ACCEL && velocityZ > 0.0f) {
                state = STATE_COAST;
            }
            break;

        case STATE_COAST:
            // Still motor-off; once the climb is nearly spent call it apogee.
            if (velocityZ <= VELOCITY_EPS) {
                state = STATE_APOGEE;
            }
            break;

        case STATE_APOGEE:
            // Vertical velocity went negative -> coming back down.
            if (velocityZ < -VELOCITY_EPS) {
                state = STATE_DESCENT;
            }
            break;

        case STATE_DESCENT:
            // Touchdown: vertical speed is ~0 and 1g is back because the rocket
            // is resting on the ground again.
            if (fabsf(velocityZ) < VELOCITY_EPS && accelZ_avg > BURNOUT_ACCEL) {
                state = STATE_LANDED;
            }
            break;

        case STATE_LANDED:
        default:
            break;
    }

}

void calculateVelocityZ(){
    // deltaTime is in milliseconds, so convert to seconds first.
    float dt = deltaTime / 1000.0f;
    // Integrate only the net vertical acceleration (remove the constant 1g the
    // sensor reports while the rocket sits on the pad).
    velocityZ += (accelZ_avg - GRAVITY) * dt;
}
