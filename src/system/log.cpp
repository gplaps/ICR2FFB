#include "log.h"

#include <iostream>
#include <set>
#include <vector>

// Global log buffer
DEFINE_MUTEX(Logger::mutex);
Logger* logger = NULL;

// Logging stuff - Keeps messages for future debugging!
// Write to log.txt
void LogMessage(const std::wstring& msg)
{
    if (!logger) { return; }

    LOCK_MUTEX(Logger::mutex);
    // Add to in-memory deque for optional UI display (if needed)
    logger->linesQueue.push_back(msg);
    UNLOCK_MUTEX(Logger::mutex);
}

void PrintToLogFile()
{
    if (!logger) { return; }

    // Copy new log message to local variable
    std::vector<std::wstring> queuedLines;
    {
        LOCK_MUTEX(Logger::mutex);
        queuedLines = logger->linesQueue;
        logger->linesQueue.clear();
        UNLOCK_MUTEX(Logger::mutex);
    }

    // Append to log file
    if (logger->file.is_open())
    {
        for (size_t i = 0; i < queuedLines.size(); ++i)
        {
            logger->file << queuedLines[i] << L'\n';
        }
        if (queuedLines.size())
        {
            logger->file.flush(); // optional, performance penalty but maybe helpful as otherwise a few log lines could be lost if the app crashes
        }
    }
    // append new log messages
    logger->lines.insert(logger->lines.end(), queuedLines.begin(), queuedLines.end());
    // ensure log stays compact
    while (logger->lines.size() > Logger::maxLogLines)
    {
        logger->lines.pop_front();
    }

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
}

Logger::Logger(const char* filename) :
    lines(),
    file(filename, std::ios::trunc) {}

Logger::~Logger()
{
    PrintToLogFile();
}
