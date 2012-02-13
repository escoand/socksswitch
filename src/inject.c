
/*
 *     Copyright (C) 2011-2012 Andreas Schönfelder
 *     https://github.com/escoand/socksswitch
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#include <windows.h>
#include <string.h>
#include <tlhelp32.h>
#include "inject.h"
#include "trace.h"

BOOL inject(int pid, const char *dll) {
    HANDLE hProc;
    FARPROC pLL;
    LPVOID radr;
    DWORD br;

    if (!pid)
	return FALSE;

    if ((pLL =
	 GetProcAddress(LoadLibrary("kernel32.dll"),
			"LoadLibraryA")) == NULL)
	return FALSE;
    if ((hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid)) == NULL)
	return FALSE;
    if ((radr =
	 VirtualAllocEx(hProc, NULL, strlen(dll), MEM_COMMIT | MEM_RESERVE,
			PAGE_EXECUTE_READWRITE)) == 0)
	return FALSE;
    if (WriteProcessMemory(hProc, radr, (LPVOID) dll, strlen(dll), &br) ==
	0)
	return FALSE;
    if (NULL ==
	CreateRemoteThread(hProc, 0, 0, (LPTHREAD_START_ROUTINE) pLL, radr,
			   0, 0))
	return FALSE;

    return TRUE;
}

DWORD getProcId(const char *path) {
    PROCESSENTRY32 pe;
    MODULEENTRY32 me;
    HANDLE snapProc;
    HANDLE snapMod;
    const char *file = strrchr(path, '\\') + 1;

    snapProc = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapProc == INVALID_HANDLE_VALUE) {
	TRACE_WARNING
	    ("failure on creating toolhelp snapshot(err:%d)\n ",
	     GetLastError());
	return FALSE;
    }

    pe.dwSize = sizeof(pe);
    me.dwSize = sizeof(me);

    /* run processes */
    while (Process32Next(snapProc, &pe)) {
	if (strcasecmp(pe.szExeFile, file) == 0) {
	    snapMod =
		CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,
					 pe.th32ProcessID);
	    if (snapMod == INVALID_HANDLE_VALUE) {
		TRACE_WARNING
		    ("failure on creating toolhelp snapshot(err:%d)\n",
		     GetLastError());
		return FALSE;
	    }

	    /* run modules */
	    while (Module32Next(snapMod, &me))
		if (strcasecmp(me.szExePath, path) == 0)
		    return pe.th32ProcessID;
	}
    }

    return FALSE;
}
