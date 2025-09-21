#pragma once

const unsigned int DEFAULT_DINPUT_GAIN     = 10000;
const double       DEFAULT_DINPUT_GAIN_DBL = static_cast<double>(DEFAULT_DINPUT_GAIN);

const double MPH_TO_MS                     = 0.44704;

const double MAX_USEFUL_G                  = 8.0;
const double TYPICAL_G                     = 4.0;

const double STANDSTILL_SPEED              = 2.0;
const double MIN_LAT_G                     = 0.1;

const double RESPONSE_THRESHOLD            = 0.02;

const double SPEED_THRESHOLD               = 20.0;
const double SPEED_SCALE_RAMP_RANGE        = 40.0;

const double SLIP_SPEED_RAMP_RANGE         = 20.0;
const double SLIP_SPEED_THRESHOLD          = SPEED_THRESHOLD;

// Physics constants from your engineer friend
const double CURVE_STEEPNESS = 1.0e-4;
const double MAX_THEORETICAL = 8500;
const double SCALE_FACTOR    = 1.20;
const double MAX_FORCE_IN_N  = MAX_THEORETICAL * SCALE_FACTOR; // 10200 - correlates to DEFAULT_DINPUT_GAIN 10000 - needs to be moved out of force calculation though, keep it at 0-1 scale

#if !defined(M_PI)
#    define M_PI 3.14159265358979323846
#endif
