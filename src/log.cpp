#include "log.h"

#include <deque>
#include <fstream>
#include <iostream>
#include <set>
#include <vector>

// Global log buffer
DEFINE_MUTEX(logMutex);
static std::deque<std::wstring> logLines;
const size_t                    maxLogLines = 1000; // Show last 1000 log lines

// Logging stuff - Keeps messages for future debugging!
// Write to log.txt
void LogMessage(const std::wstring& msg)
{
    LOCK_MUTEX(logMutex);

    // Add to in-memory deque for optional UI display (if needed)
    logLines.push_back(msg);
    while (logLines.size() > maxLogLines)
    {
        logLines.pop_front();
    }

    // Append to log.txt
    std::wofstream logFile("log.txt", std::ios::app);
    if (logFile.is_open())
    {
        logFile << msg << L'\n';
    }

    UNLOCK_MUTEX(logMutex);
}

void PrintToLogFile()
{
    //Print log data
    LOCK_MUTEX(logMutex);

    const unsigned int        maxDisplayLines = 1; //how many lines to display
    std::vector<std::wstring> recentUniqueLines;
    std::set<std::wstring>    seen;

    // Go backward to find most recent unique messages
    for (std::deque<std::wstring>::reverse_iterator it = logLines.rbegin(); it != logLines.rend() && recentUniqueLines.size() < maxDisplayLines; ++it)
    {
        if (seen.insert(*it).second)
        {
            recentUniqueLines.push_back(*it);
        }
    }

    for (std::vector<std::wstring>::reverse_iterator it = recentUniqueLines.rbegin(); it != recentUniqueLines.rend(); ++it)
    {
        const std::wstring& line   = *it;
        std::wstring        padded = line;
        padded.resize(80, L' '); // pad to 80 characters to clear old line leftovers
        std::wcout << padded << L"\n";
    }

    UNLOCK_MUTEX(logMutex);
}
