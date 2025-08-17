#include "log.h"

#include <deque>
#include <fstream>
#include <iostream>
#include <mutex>
#include <unordered_set>

// Global log buffer
static std::mutex               logMutex;
static std::deque<std::wstring> logLines;
const size_t                    maxLogLines = 1000; // Show last 1000 log lines

// Logging stuff - Keeps messages for future debugging!
// Write to log.txt
void LogMessage(const std::wstring& msg)
{
    std::lock_guard<std::mutex> lock(logMutex);

    // Add to in-memory deque for optional UI display (if needed)
    logLines.push_back(msg);
    while (logLines.size() > maxLogLines)
        logLines.pop_front();

    // Append to log.txt
    std::wofstream logFile("log.txt", std::ios::app);
    if (logFile.is_open())
        logFile << msg << std::endl;
}

void PrintToLogFile()
{
    //Print log data
    std::lock_guard<std::mutex>      lock(logMutex);
    const unsigned int               maxDisplayLines = 1; //how many lines to display
    std::vector<std::wstring>        recentUniqueLines;
    std::unordered_set<std::wstring> seen;

    // Go backward to find most recent unique messages
    for (auto it = logLines.rbegin(); it != logLines.rend() && recentUniqueLines.size() < maxDisplayLines; ++it)
    {
        if (seen.insert(*it).second)
            recentUniqueLines.push_back(*it);
    }

    for (auto it = recentUniqueLines.rbegin(); it != recentUniqueLines.rend(); ++it)
    {
        const auto&  line   = *it;
        std::wstring padded = line;
        padded.resize(80, L' '); // pad to 80 characters to clear old line leftovers
        std::wcout << padded << L"\n";
    }
}
