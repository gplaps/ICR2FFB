#pragma once
#include "project_dependencies.h"

#include <string>

void LogMessage(const std::wstring& msg);
void PrintToLogFile();

#if !defined(HAS_STL_THREAD_MUTEX)
extern HANDLE logMutex;
#endif
