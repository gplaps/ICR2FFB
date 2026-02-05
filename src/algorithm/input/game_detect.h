#pragma once

#include "project_dependencies.h" // IWYU pragma: keep

#include "game_version.h"

DWORD FindProcessIdByWindow(const std::wstring& customKeywords);
Game  ScanSignature(HANDLE processHandle);
