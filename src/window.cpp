#include "window.h"

#include "project_dependencies.h"

#include "log.h"
#include "main.h"

#include <shellapi.h>

#include <cstdlib>
#include <iostream>

// Check Admin rights
static bool IsRunningAsAdmin()
{
    BOOL isAdmin                         = FALSE;
    PSID administratorsGroup             = NULL;

    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &administratorsGroup))
    {
        CheckTokenMembership(NULL, administratorsGroup, &isAdmin);
        FreeSid(administratorsGroup);
    }

    return isAdmin == TRUE;
}

static int RestartAsAdmin()
{
    // Get the current executable path
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);

    // Use ShellExecuteW to restart with "runas" (admin prompt)
    HINSTANCE result = ShellExecuteW(
        NULL,         // Parent window
        L"runas",     // Operation (request admin)
        exePath,      // Program to run
        NULL,         // Command line arguments
        NULL,         // Working directory
        SW_SHOWNORMAL // Show window normally
    );

    // Check if the restart was successful
    if (reinterpret_cast<intptr_t>(result) > 32)
    {
        // Success - the new admin instance is starting, exit this one
        std::wcout << L"[INFO] Restarting with administrator privileges..." << L'\n';
        return 0;
    }
    else
    {
        // Failed - user probably clicked "No" on UAC prompt
        std::wcout << L"[ERROR] Failed to restart as administrator." << L'\n';
        std::wcout << L"[ERROR] Please right-click the program and select 'Run as administrator'" << L'\n';
        return 1;
    }
}

int CheckAndRestartAsAdmin()
{
    if (!IsRunningAsAdmin())
    {
        std::wcout << L"===============================================" << L'\n';
        std::wcout << L"    ICR2 FFB Program - Admin Rights Required" << L'\n';
        std::wcout << L"===============================================" << L'\n';
        std::wcout << L"" << L'\n';
        std::wcout << L"This program requires administrator privileges to:" << L'\n';
        std::wcout << L"  - Access DirectInput force feedback devices" << L'\n';
        std::wcout << L"  - Control console display properly" << L'\n';
        std::wcout << L"  - Ensure reliable wheel communication" << L'\n';
        std::wcout << L"" << L'\n';
        std::wcout << L"Would you like to restart as administrator? (y/n): ";

        wchar_t response = 0;
        std::wcin >> response;

        if (response == L'y' || response == L'Y')
        {
            if(RestartAsAdmin())
            {
                // If we get here, the restart failed
                std::wcout << L"Press any key to exit..." << L'\n';
                std::cin.get();
                return 1;
            }
            return 1;
        }
        else
        {
            std::wcout << L"" << L'\n';
            std::wcout << L"Cannot continue without administrator privileges." << L'\n';
            std::wcout << L"Please restart the program as administrator." << L'\n';
            std::wcout << L"Press any key to exit..." << L'\n';
            std::wcin.ignore();
            std::wcin.get();
            return 1;
        }
    }

    return 0;
}

// Console drawing stuff
// little function to help with display refreshing
// moves cursor to top without refreshing the screen

static void SetConsoleWindowSize()
{
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE)
    {
        LogMessage(L"[ERROR] Failed to get console handle");
        return;
    }

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hOut, &csbi))
    {
        LogMessage(L"[ERROR] Failed to get console buffer info");
        return;
    }

    const COORD bufferSize = {120, 200}; // scrollable size
    if (!SetConsoleScreenBufferSize(hOut, bufferSize))
    {
        LogMessage(L"[WARNING] Failed to set console buffer size");
    }

    const SMALL_RECT windowSize = {0, 0, 119, 39}; // window size (note: 119, not 120)
    if (!SetConsoleWindowInfo(hOut, TRUE, &windowSize))
    {
        LogMessage(L"[WARNING] Failed to set console window size");
    }
    else
    {
        LogMessage(L"[INFO] Console window size set successfully");
    }
}

// Prevent lockup if window is clicked
static void DisableConsoleQuickEdit()
{
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    if (hInput == INVALID_HANDLE_VALUE)
    {
        LogMessage(L"[ERROR] Failed to get input handle");
        return;
    }

    DWORD mode = 0;
    if (!GetConsoleMode(hInput, &mode))
    {
        LogMessage(L"[ERROR] Failed to get console mode");
        return;
    }


    mode &= static_cast<DWORD>(~(ENABLE_QUICK_EDIT_MODE | ENABLE_INSERT_MODE));
    mode |= ENABLE_EXTENDED_FLAGS;

    if (!SetConsoleMode(hInput, mode))
    {
        LogMessage(L"[ERROR] Failed to set console mode");
    }
    else
    {
        LogMessage(L"[INFO] Console Quick Edit Mode disabled");
    }
}

void MoveCursorToTop()
{
    HANDLE      hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    const COORD topLeft  = {0, 0};
    SetConsoleCursorPosition(hConsole, topLeft);
}

void MoveCursorToLine(short lineNumber)
{
    HANDLE      hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    const COORD pos      = {0, lineNumber};
    SetConsoleCursorPosition(hConsole, pos);
}

static void HideConsoleCursor()
{
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE)
    {
        LogMessage(L"[ERROR] Failed to get console handle for cursor");
        return;
    }

    CONSOLE_CURSOR_INFO cursorInfo;
    if (!GetConsoleCursorInfo(hOut, &cursorInfo))
    {
        LogMessage(L"[ERROR] Failed to get cursor info");
        return;
    }

    cursorInfo.bVisible = FALSE;
    if (!SetConsoleCursorInfo(hOut, &cursorInfo))
    {
        LogMessage(L"[ERROR] Failed to hide cursor");
    }
    else
    {
        LogMessage(L"[INFO] Cursor hidden successfully");
    }
}

static BOOL WINAPI ConsoleHandler(DWORD CEvent) NO_EXCEPT
{
    switch (CEvent)
    {
        case CTRL_C_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            shouldExit = true;
            break;
        default: break;
    }
    return TRUE;
}

int InitConsole()
{
    SetConsoleWindowSize();
    HideConsoleCursor();
    DisableConsoleQuickEdit();
    if (SetConsoleCtrlHandler(
            static_cast<PHANDLER_ROUTINE>(ConsoleHandler), TRUE) == FALSE)
    {
        LogMessage(L"[ERROR] Unable to install keyboard ctrl handler");
        return -1;
    }

    return 0;
}
