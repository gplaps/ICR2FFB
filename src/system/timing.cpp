#include "timing.h"

#include "project_dependencies.h" // IWYU pragma: keep

#include "log.h"
#include "string_utilities.h" // IWYU pragma: keep
#if defined(HAS_STL_THREAD_MUTEX)
#    include <thread>
#endif
#include <mmsystem.h>

#include <cmath>

static LARGE_INTEGER start, end, frequency;
#define PUT_THREADS_TO_SLEEP
#if defined(PUT_THREADS_TO_SLEEP)
const int DesiredTimerResolution = 1;
// default scheduler resolution in Windows is 15.6ms - to ensure the calculated sleep time is not constantly overrun, adjust to minimum configurable 1ms timing. This may have negative effects on power consumptions!
// in case of timing issues (log.txt contains frequent timing reports), fall back to active wait/spinning by removing the PUT_THREADS_TO_SLEEP define.
// thread being late was observed when changing window focus and does not need to be fixed
// if it does happen constantly - e.g. between the at the time of writing - [DEBUG] tire load messages - it needs to be addressed
// also consider calling timeBeginPeriod only if FFB output is required (game not paused). otherwise jitter may be tolerable
static const double SLACK_TIME_MS = 0.5; // wake up thread this amount of time before scheduled time resulting in active wait / polling == likely on time, but wasting some resources ... set to zero, to accept slightly late
#endif

#define FRAMES_PER_SECOND(x) (1000.0 / static_cast<double>(x))

Timing::Timing(const FFBConfig& config) :
    ffb(FRAMES_PER_SECOND(60), config.GetBool(L"base", L"verbose")),
    telemetry(FRAMES_PER_SECOND(60)),
    render(FRAMES_PER_SECOND(15.0), false)
{
#if defined(PUT_THREADS_TO_SLEEP)
    timeBeginPeriod(DesiredTimerResolution);
#endif
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);
}

Timing::~Timing()
{
#if defined(PUT_THREADS_TO_SLEEP)
    timeEndPeriod(DesiredTimerResolution);
#endif
}

double TimeSinceStartInMs()
{
    QueryPerformanceCounter(&end);
    return static_cast<double>(end.QuadPart - start.QuadPart) * 1000.0 / static_cast<double>(frequency.QuadPart);
}

bool ThreadTimer::ready()
{
    const double currentTime = TimeSinceStartInMs();
    if (currentTime >= nextTime)
    {
        const double lateMs = currentTime - nextTime;
        if (report && lateMs > static_cast<double>(DesiredTimerResolution * 2))
        {
            // there is little you can do to OS threading being late, at least log about and to notice if adjustments are necessary
            LogMessage(L"FFB thread was late at: " + std::to_wstring(currentTime) + L" D: " + std::to_wstring(lateMs) + L" I: " + std::to_wstring(interval));
        }
        nextTime = currentTime + interval;
        return true;
    }
    return false;
}

void ThreadTimer::schedule() const
{
#if defined(PUT_THREADS_TO_SLEEP) // predict when thread needs to wake up - results in less wasteful polling
    const double currentTime = TimeSinceStartInMs();
    const double waitTime    = nextTime - currentTime - SLACK_TIME_MS;
    const int    waitTimeI   = static_cast<int>(std::floor(waitTime));
    const double thresholdT  = std::floor(interval - static_cast<double>(DesiredTimerResolution) - SLACK_TIME_MS);
    const bool   notOverTime = waitTimeI > static_cast<int>(thresholdT);
    if (waitTimeI >= 1 && notOverTime)
    {
        // if (report) // logging can slow things done significantly
        // {
        //     LogMessage(L"FFB thread sleeping at: " + std::to_wstring(currentTime) + L"ms for " + std::to_wstring(waitTime) +  + L"ms threshold: " + std::to_wstring(thresholdT)L"ms - interval " + std::to_wstring(interval) + L"ms");
        // }
#    if defined(HAS_STL_THREAD_MUTEX)
        std::this_thread::sleep_for(std::chrono::milliseconds(waitTimeI));
        // std::this_thread::yield();
#    else
        Sleep(static_cast<DWORD>(waitTimeI));
        // Sleep(0); // == yield
#    endif
    }
#else
    // std::this_thread::sleep_for(std::chrono::milliseconds(1));
#endif
}
