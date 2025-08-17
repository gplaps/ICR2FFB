#pragma once
#include "project_dependencies.h"

#include <string>

void LogMessage(const std::wstring& msg);
void PrintToLogFile();

DECLARE_MUTEX(logMutex);
