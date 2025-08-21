#pragma once
#include "project_dependencies.h"

#include <deque>
#include <fstream>
#include <string>

void LogMessage(const std::wstring& msg);
void PrintToLogFile();

struct Logger
{
    explicit Logger(const char* filename) :
        lines(),
        file(filename, std::ios::trunc) {}
    std::deque<std::wstring> lines;
    std::deque<std::wstring> linesQueue;
    std::wofstream           file;

    static DECLARE_MUTEX(mutex);
    static const size_t maxLogLines = 1000; // Show last 1000 log lines
};
extern Logger* logger;
