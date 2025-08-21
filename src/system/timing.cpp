#include "timing.h"

#include "project_dependencies.h" // IWYU pragma: keep

#include "log.h"
#include "string_utilities.h" // IWYU pragma: keep
#if defined(HAS_STL_THREAD_MUTEX)
#    include <thread>
#endif

static LARGE_INTEGER start, end, frequency;

#define CALCULATE_WAIT
#if defined(CALCULATE_WAIT)
static const double SLACK_TIME_MS = 0.5; // wake up thread this amount of time before scheduled time resulting in active wait / polling == likely on time, but wasting resources ... set to zero, to accept slightly late
#endif

#define FRAMES_PER_SECOND(x) (1000.0 / static_cast<double>(x))

Timing::Timing() :
    ffb(FRAMES_PER_SECOND(60), true),
    telemetry(FRAMES_PER_SECOND(60)),
    render(FRAMES_PER_SECOND(15.0), false)
{
    //timing
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);
}

static double timeSinceStartInMs()
{
    QueryPerformanceCounter(&end);
    return static_cast<double>(end.QuadPart - start.QuadPart) * 1000.0 / static_cast<double>(frequency.QuadPart);
}

bool ThreadTimer::ready()
{
    const double currentTime = timeSinceStartInMs();
    if (currentTime >= nextTime)
    {
        const double lateMs = (currentTime - nextTime);
        if (report && lateMs > 1.0) // there is little you can do to OS threading being late, at least log about and to notice if adjustments are necessary
        {
            LogMessage(L"T " + std::to_wstring(currentTime) + L" D: " + std::to_wstring(lateMs) + L" I: " + std::to_wstring(interval));
        }
        nextTime = currentTime + interval;
        return true;
    }
    return false;
}

void ThreadTimer::schedule() const
{
#if defined(CALCULATE_WAIT) // predict when thread needs to wake up - results in less wasteful polling
    const double       currentTime = timeSinceStartInMs();
    const unsigned int waitTime    = static_cast<unsigned int>(nextTime - currentTime - SLACK_TIME_MS);
    if (waitTime >= 1)
    {
#    if defined(HAS_STL_THREAD_MUTEX)
        std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));
        // std::this_thread::yield();
#    else
        Sleep(static_cast<DWORD>(waitTime));
        // Sleep(0); // == yield
#    endif
    }
#else
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
#endif
}
