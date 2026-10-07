#ifndef QUACK_WIN_UPDATE_H
#define QUACK_WIN_UPDATE_H

#if defined(_WIN32) || defined(_WIN64)

#include <windows.h>

/* Starts a background GitHub Releases update check. */
void win_update_start_check(HWND owner);

#endif

#endif
