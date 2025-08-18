#include "log.h"

#include <iostream>
#include <set>
#include <vector>

// Global log buffer
Logger* logger = NULL;

// Logging stuff - Keeps messages for future debugging!
// Write to log.txt
void LogMessage(const std::wstring& msg)
{
    if (!logger) { return; }

    LOCK_MUTEX(Logger::logMutex);

    // Add to in-memory deque for optional UI display (if needed)
    logger->lines.push_back(msg);
    while (logger->lines.size() > Logger::maxLogLines)
    {
        logger->lines.pop_front();
    }

    // Append to log.txt
    if (logger->file.is_open())
    {
        logger->file << msg << L'\n';
    }

    UNLOCK_MUTEX(Logger::logMutex);
}

void PrintToLogFile()
{
    if (!logger) { return; }

    //Print log data
    LOCK_MUTEX(Logger::logMutex);

    const unsigned int        maxDisplayLines = 1; //how many lines to display
    std::vector<std::wstring> recentUniqueLines;
    std::set<std::wstring>    seen;

    // Go backward to find most recent unique messages
    for (std::deque<std::wstring>::reverse_iterator it = logger->lines.rbegin(); it != logger->lines.rend() && recentUniqueLines.size() < maxDisplayLines; ++it)
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

    UNLOCK_MUTEX(Logger::logMutex);
}
