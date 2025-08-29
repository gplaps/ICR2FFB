#pragma once

#include "project_dependencies.h"

#include "game_version.h"

#include <utility>

DWORD FindProcessIdByWindow(GameVersion version);
std::pair<uintptr_t, GameVersion> ScanSignature(HANDLE processHandle, GameVersion version);
