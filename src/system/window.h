#pragma once

// Check Admin rights
bool CheckAndRestartAsAdmin();

// Console drawing stuff
// little function to help with display refreshing
// moves cursor to top without refreshing the screen
void MoveCursorToTop();
void MoveCursorToLine(short lineNumber);
int  InitConsole();
