#include <windows.h>
#include <tlhelp32.h>
#include <shlwapi.h>
#include <conio.h>
#include <stdio.h>
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
	if (StrStrI(pe.szExeFile, file)) {
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
		if (StrStrI(me.szExePath, path))
		    return pe.th32ProcessID;
	}
    }

    return FALSE;
}
