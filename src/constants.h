#pragma once

const unsigned int DEFAULT_DINPUT_GAIN=10000;

const double MPH_TO_MS=0.44704;

const double MAX_USEFUL_G=8.0;
const double TYPICAL_G=4.0;

const double STANDSTILL_SPEED=2.0;
const double MIN_LAT_G=0.1;

const double RESPONSE_THRESHOLD=0.02;

const double SPEED_THRESHOLD = 20.0;
const double SPEED_SCALE_RAMP_RANGE=40.0;

const double SLIP_SPEED_RAMP_RANGE=20.0;
const double SLIP_SPEED_THRESHOLD=SPEED_THRESHOLD;

#if !defined(M_PI)
#define M_PI 3.14159265358979323846
#endif
