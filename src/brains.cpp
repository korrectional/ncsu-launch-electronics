#include "brains.hpp"
#include "globals.hpp"

#include <math.h>

// Flight thresholds -- tune these to the vehicle.
// The BNO085 accelerometer is gone, so every threshold below is expressed in
// terms of barometric height and its rate of change instead of acceleration.
static constexpr float ALTITUDE_LIFTOFF_M = 3.0f;   // m above the pad -> boost
static constexpr float CLIMB_RATE_BURNOUT  = 5.0f;  // m/s, climb rate falling to this = coast
static constexpr float VELOCITY_APOGEE_EPS = 0.5f;  // m/s, "vertical velocity ~ 0"
static constexpr float ALTITUDE_LANDED_M   = 2.0f;  // m above the pad -> back on the ground

// A transition condition must hold on several consecutive loop iterations
// before the state machine acts on it (added in PR #14). The flight loop runs
// ~20 Hz, so 3 confirmations is ~150 ms of agreement -- long enough to reject a
// single noisy barometer sample, short enough not to delay a real event.
static constexpr int STATE_CONFIRM_COUNT = 3;
static int conditionCounter = 0;

// Returns true once `condition` has held for STATE_CONFIRM_COUNT calls in a
// row. The counter is cleared on any false sample, and is consumed when the
// condition is confirmed, so the next state starts counting from zero.
static bool confirmed(bool condition) {
    if (!condition) {
        conditionCounter = 0;
        return false;
    }
    if (++conditionCounter < STATE_CONFIRM_COUNT) {
        return false;
    }
    conditionCounter = 0;
    return true;
}

// ---------------------------------------------------------------------------
// Rolling average helper (unchanged)
// ---------------------------------------------------------------------------
float calculateAverages(float list[3], int size){
    // TODO store last 3 data readings for all important variables and create an
    // average for those last 3 (eg pressure_hPa_avg).
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

// ---------------------------------------------------------------------------
// Barometric height
// ---------------------------------------------------------------------------
// International barometric formula, referenced to the pressure captured on the
// pad so the result is height above the pad (AGL):
//
//     h = 44330 * (1 - (P / P0)^(1/5.255))
//
// P0 = groundPressure_hPa, P = the smoothed pressure reading.
void updateAltitude(float pressure_hPa_now){
    if (groundPressure_hPa <= 0.0f || pressure_hPa_now <= 0.0f) {
        return;  // no pad reference yet, or the sensor has not produced a value
    }

    altitude_m = 44330.0f * (1.0f - powf(pressure_hPa_now / groundPressure_hPa, 1.0f / 5.255f));

    // The raw DPS310 reading is noisy at the sub-metre level and noise in the
    // height directly becomes noise in the derived velocity, so smooth the
    // height before differentiating it.
    static int altitudeIndex = 0;
    altitude_list[altitudeIndex] = altitude_m;
    altitudeIndex = (altitudeIndex + 1) % 3;

    static int altitudeReadings = 1;
    if (altitudeReadings >= 3) {
        altitude_m_avg = calculateAverages(altitude_list, 3);
    } else if (altitudeReadings == 2) {
        altitude_m_avg = calculateAverages(altitude_list, 2);
        altitudeReadings++;
    } else {
        altitude_m_avg = altitude_m;
        altitudeReadings++;
    }

    if (altitude_m_avg > maxAltitude_m) {
        maxAltitude_m = altitude_m_avg;
    }
}

// ---------------------------------------------------------------------------
// Flight state machine
// ---------------------------------------------------------------------------
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
            // On the pad until the barometer shows we have actually climbed.
            if (confirmed(altitude_m_avg > ALTITUDE_LIFTOFF_M && velocityZ > VELOCITY_APOGEE_EPS)) {
                state = STATE_BOOST;
            }
            break;

        case STATE_BOOST:
            // "Burnout" is now read as the climb rate falling off; without an
            // accelerometer there is no thrust measurement to key off.
            if (confirmed(velocityZ < CLIMB_RATE_BURNOUT)) {
                state = STATE_COAST;
            }
            break;

        case STATE_COAST:
            // Still climbing; once the climb is nearly spent call it apogee.
            if (confirmed(velocityZ <= VELOCITY_APOGEE_EPS)) {
                state = STATE_APOGEE;
            }
            break;

        case STATE_APOGEE:
            // Vertical velocity went negative -> coming back down.
            if (confirmed(velocityZ < -VELOCITY_APOGEE_EPS)) {
                state = STATE_DESCENT;
            }
            break;

        case STATE_DESCENT:
            // Touchdown: vertical speed is ~0 and we are back near pad height.
            if (confirmed(fabsf(velocityZ) < VELOCITY_APOGEE_EPS && altitude_m_avg < ALTITUDE_LANDED_M)) {
                state = STATE_LANDED;
            }
            break;

        case STATE_LANDED:
        default:
            break;
    }

}

// ---------------------------------------------------------------------------
// Vertical velocity, differentiated from the barometric height
// ---------------------------------------------------------------------------
// This replaces the old accelerometer integration. deltaTime is in
// milliseconds, so convert to seconds first. Differentiating the smoothed
// height also gives us the climb rate needed for boost/coast detection.
void calculateVelocityZ(){
    float dt = deltaTime / 1000.0f;
    if (dt <= 0.0f) {
        return;
    }

    velocityZ = (altitude_m_avg - lastAltitude_m) / dt;
    lastAltitude_m = altitude_m_avg;
}
