#include "movement_detection.h"

#include "log.h"
#include <cfloat>

MovementDetector::MovementDetector() :
    lastDlong(0.0),
    noMovementFrames(0),
    isPaused(true),
    pauseForceSet(false),
    isFirstReading(true),
    movementThreshold(10),
    movementThreshold_value(0.001) {}

MovementDetector::MovementState MovementDetector::Calculate(const RawTelemetry& current, const RawTelemetry& /*previous*/, double deltaTimeMs)
{
    if (isFirstReading)
    {
        lastDlong      = current.pos.dlong; // Set baseline from first real data
        isFirstReading = false;
        pauseForceSet  = true;
        return MS_IDLE;
    }

    const double dtInv = deltaTimeMs < 0.001 ? DBL_MAX : 1.0 / deltaTimeMs; // div-by-zero protection
    const bool isStationary = std::abs(current.pos.dlong - lastDlong) * dtInv < movementThreshold_value; // this is time dependent, so liekly put timestamps into RawTelemetry to properly calculate from input data instead of processing time

    if (isStationary)
    {
        noMovementFrames++;
        if (noMovementFrames >= movementThreshold && !isPaused)
        {
            isPaused      = true;
            pauseForceSet = false; // ← Reset when entering pause
            LogMessage(L"[INFO] Game paused detected - sending zero force");
        }
    }
    else
    {
        if (isPaused || noMovementFrames > 0)
        {
            noMovementFrames = 0;
            if (isPaused)
            {
                isPaused      = false;
                pauseForceSet = false; // ← Reset when exiting pause
                LogMessage(L"[INFO] Game resumed - restoring normal forces");
            }
        }
        lastDlong = current.pos.dlong;
    }

    // If paused, send zero force and return
    if (isPaused)
    {
        pauseForceSet = true;
        return MS_IDLE;
    }

    // Low speed filtering
    if (current.speed_mph < 5.0)
    {
        // static bool wasLowSpeed = false;
        // if (!wasLowSpeed) {
        //     // Send zero force when entering low speed
        //     wasLowSpeed = true;
        // }
        return MS_IDLE;
    }

    // static bool wasLowSpeed = false;
    // if (wasLowSpeed)
    //     wasLowSpeed = false;  // Reset when speed picks up

    return MS_DRIVING;

    // unused and redundant code - likely delete
    /*
        // Auto-pause force if not moving
        // I think this is broken or I could detect pause in a better way
        // Maybe DLONG not moving?
        bool isStationary = std::abs(current.dlong - previous.dlong) < 0.01;
        if (isStationary) {
            noMovementFrames++;
            if (noMovementFrames >= movementThreshold && !effectPaused) {
                //add damper and spring?
                constantForceEffect->Stop();
                effectPaused = true;
                LogMessage(L"[INFO] FFB paused due to no movement");
            }
        }
        else {
            noMovementFrames = 0;
            //  if (effectPaused) {
                //add damper and spring?
            //      constantForceEffect->Start(1, 0);
            //      effectPaused = false;
            //      std::wcout << L"FFB resumed\n";
            //  }
            // Added Alpha v0.6 to prevent issues with Moza?
            if (effectPaused) {
                HRESULT startHr = constantForceEffect->Start(1, 0);
                if (FAILED(startHr)) {
                    LogMessage(L"[ERROR] Failed to restart constant force: 0x" + std::to_wstring(startHr));
                }
                else {
                    effectPaused = false;
                    LogMessage(L"[INFO] FFB resumed");
                }
            }   
        }
    */
}
