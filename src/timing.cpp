#include "timing.h"

#include <thread>

static LARGE_INTEGER start, end, frequency;

#define FRAMES_PER_SECOND(x) (1.0 / static_cast<double>(x))

static const double SLACK_TIME_MS = 0.5; // wake up thread this amount of time before scheduled time resulting in active wait == likely on time, but wasting resources ... set to zero, to accept slightly late

Timing::Timing() :
    ffb(FRAMES_PER_SECOND(60)),
    telemetry(FRAMES_PER_SECOND(60)),
    render(FRAMES_PER_SECOND(15.0), false)
{
    //timing
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);
}

static double getPerformanceCounterTime()
{
    QueryPerformanceCounter(&end);
    return static_cast<double>(end.QuadPart - start.QuadPart) * 1000.0 / static_cast<double>(frequency.QuadPart);
}

bool ThreadTimer::canStart()
{
    const double currentTime = getPerformanceCounterTime();
    if (currentTime >= nextTime)
    {
        nextTime = currentTime + interval;
        return true;
    }
    return false;
}

void ThreadTimer::finished() const
{
    const double currentTime = getPerformanceCounterTime();
    const double waitTime    = nextTime - currentTime - (keepGoodTiming ? SLACK_TIME_MS : 0.0);
    if (waitTime > 0.0)
    {
#if defined(HAS_STL_THREAD_MUTEX)
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(waitTime)));
        // std::this_thread::yield();
#else
        Sleep(static_cast<int>(waitTime));
        // Sleep(0); // == yield
#endif
    }
}
